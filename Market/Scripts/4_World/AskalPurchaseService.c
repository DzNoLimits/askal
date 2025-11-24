	// ==========================================
// AskalPurchaseService - Serviço de compra (4_World - acesso a PlayerBase)
// ==========================================

class AskalPurchaseService
{
	// Processar compra COM quantidade e conteúdo customizados
	static bool ProcessPurchaseWithQuantity(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId, float itemQuantity, int quantityType, int contentType, string traderName = "")
	{
		if (!identity)
		{
			Print("[AskalPurchase] ❌ Player identity não encontrada");
			return false;
		}

		if (!steamId || steamId == "")
		{
			steamId = identity.GetPlainId();
			if (!steamId || steamId == "")
				steamId = identity.GetId();
		}

		// Obter player
		PlayerBase player = GetPlayerFromIdentity(identity);
		if (!player)
		{
			Print("[AskalPurchase] ❌ Player não encontrado");
			return false;
		}

		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
		// VALIDAÇÃO SERVIDOR: Verificar se quantidade solicitada é válida
		if (!ValidateQuantity(itemClass, itemQuantity, quantityType))
		{
			Print("[AskalPurchase] ❌ MARKET_PURCHASE_REJECT reason=invalid_quantity player=" + steamId + " item=" + itemClass + " requested_qty=" + itemQuantity);
			return false;
		}

		// Calcular preço autoritativo considerando quantidade (para stackables)
		int authoritativePrice = ComputeItemTotalPriceWithQuantity(itemClass, itemQuantity, quantityType);
		if (authoritativePrice <= 0)
		{
			Print("[AskalPurchase] ❌ Preço autoritativo inválido para " + itemClass + ": " + authoritativePrice);
			return false;
		}
		if (price != authoritativePrice)
		{
			Print("[AskalPurchase] ⚠️ Ajustando preço informado pelo cliente. Recebido: " + price + " | Autoritativo: " + authoritativePrice);
			price = authoritativePrice;
		}

		// ITER-1: Reserve funds atomically BEFORE creating item (prevents TOCTOU)
		if (!AskalPlayerBalance.ReserveFunds(steamId, price, currencyId))
		{
			Print("[AskalPurchase] ❌ Falha ao reservar funds: " + steamId + " | Amount: " + price);
			return false;
		}
		
		// Verificar se é veículo (CarScript ou BoatScript)
		// Veículos são spawnados no mundo, não no inventário
		EntityAI createdItem = NULL;
		if (AskalVehicleSpawnService.IsVehicle(itemClass))
		{
			Print("[AskalPurchase] 🚗 Detectado veículo: " + itemClass);
			createdItem = AskalVehicleSpawnService.SpawnVehicleInWorld(player, itemClass, traderName);
			if (!createdItem)
			{
				Print("[AskalPurchase] ❌ Não foi possível spawnar veículo: " + itemClass);
				Print("[AskalPurchase] ❌ Motivo: SEM PONTO DE ENTREGA DISPONÍVEL");
				// ITER-1: Release reservation if vehicle spawn fails
				AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
				return false;
			}
			
			// Aplicar attachments padrão aos veículos
			Print("[AskalPurchase] 🔧 Aplicando attachments ao veículo: " + itemClass);
			
			// Obter attachments do dataset
			array<string> attachments = GetAttachmentsForItem(itemClass);
			Print("[AskalPurchase] 📋 Attachments encontrados: " + attachments.Count().ToString());
			if (attachments && attachments.Count() > 0)
			{
				for (int i = 0; i < attachments.Count(); i++)
				{
					Print("[AskalPurchase]   - Attachment " + i.ToString() + ": " + attachments.Get(i));
				}
			}
			
			AttachDefaultAttachments(createdItem, itemClass);
		}
		else
		{
			// Criar item normal (não-veículo)
			createdItem = CreateSimpleItem(player, itemClass, itemQuantity, quantityType, contentType);
			if (!createdItem)
			{
				Print("[AskalPurchase] ❌ Não foi possível criar item: " + itemClass);
				// ITER-1: Release reservation if item creation fails
				AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
				return false;
			}
			
			// Attachments padrão (apenas para itens não-veículos)
			AttachDefaultAttachments(createdItem, itemClass);
		}

		// ITER-1: Confirm reservation (convert to actual deduction)
		if (!AskalPlayerBalance.ConfirmReservation(steamId, price, currencyId))
		{
			Print("[AskalPurchase] ❌ Erro ao confirmar reserva");
			GetGame().ObjectDelete(createdItem);
			AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
			return false;
		}

		Print("[AskalPurchase] ORDER_PLACED steamId=" + steamId + " item=" + itemClass + " price=" + price);
		Print("[AskalPurchase] ✅ Compra realizada!");
		Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Tipo: " + quantityType);

		return true;
	}

	// Criar item simples com busca inteligente em 3 etapas
	// 1. Attachment slots compatíveis (recursivo: player slots até itens no inventário)
	// 2. Inventory slots (cargo de containers - recursivo)
	// 3. Mãos do player (se ocupadas, falha a compra)
	static EntityAI CreateSimpleItem(PlayerBase player, string itemClass, float quantity, int quantityType, int contentType)
	{
		Print("[AskalPurchase] 🛠️ Criando item: " + itemClass);

		EntityAI createdItem = NULL;

		// ETAPA 1: Buscar attachment slots compatíveis (recursivo)
		// Apenas para itens que podem ser attachments (pular se não for necessário)
		Print("[AskalPurchase] 🔍 ETAPA 1: Buscando attachment slots compatíveis...");
		createdItem = FindCompatibleAttachmentSlotRecursive(player, itemClass);
		if (createdItem)
		{
			Print("[AskalPurchase] ✅ Item encaixado em attachment slot");
		}
		else
		{
			Print("[AskalPurchase] ⚠️ ETAPA 1: Nenhum attachment slot compatível encontrado");
			
			// ETAPA 2: Buscar inventory slots (cargo de containers - recursivo)
			Print("[AskalPurchase] 🔍 ETAPA 2: Buscando inventory slots em containers...");
			createdItem = FindInventorySlotRecursive(player, itemClass);
			if (createdItem)
			{
				Print("[AskalPurchase] ✅ Item colocado em inventory slot");
			}
			else
			{
				Print("[AskalPurchase] ⚠️ ETAPA 2: Nenhum inventory slot disponível");
				
				// ETAPA 3: Tentar colocar nas mãos do player
				Print("[AskalPurchase] 🔍 ETAPA 3: Tentando colocar nas mãos...");
				if (player.GetHumanInventory())
				{
					createdItem = player.GetHumanInventory().CreateInHands(itemClass);
					if (createdItem)
					{
						Print("[AskalPurchase] ✅ Item colocado nas mãos");
					}
					else
					{
						Print("[AskalPurchase] ❌ ETAPA 3: Mãos ocupadas - compra não pode ser concluída");
						return NULL;
					}
				}
				else
				{
					Print("[AskalPurchase] ❌ ETAPA 3: GetHumanInventory() retornou NULL");
					return NULL;
				}
			}
		}

		// Aplicar quantidade apenas para tipos que funcionam
		ItemBase itemBase = ItemBase.Cast(createdItem);
		if (itemBase)
		{
			if (quantityType == 1 && itemBase.IsMagazine()) // MAGAZINE
			{
				Magazine mag = Magazine.Cast(itemBase);
				if (mag)
				{
					int ammoCount = Math.Round(Math.Clamp(quantity, 0, mag.GetAmmoMax()));
					mag.ServerSetAmmoCount(ammoCount);
					
					// Verificar quantidade aplicada
					int actualAmmo = mag.GetAmmoCount();
					Print("[AskalPurchase] AMMO_SPAWN qty=" + actualAmmo + " type=" + itemClass + " player=" + player.GetIdentity().GetPlainId());
					
					if (actualAmmo != ammoCount)
					{
						Print("[AskalPurchase] ⚠️ Quantidade de munição não corresponde! Solicitado: " + ammoCount + " | Atual: " + actualAmmo);
					}
					else
					{
						Print("[AskalPurchase] 🔫 Magazine: " + ammoCount + " balas");
					}
				}
			}
			else if (quantityType == 2 && itemBase.HasQuantity()) // STACKABLE
			{
				// CRÍTICO: Verificar se é munição ANTES de calcular limites
				// Munições usam GetAmmoMax() em vez de GetQuantityMax()
				Ammunition_Base ammoPile = Ammunition_Base.Cast(itemBase);
				float clampedQty;
				
				if (ammoPile && ammoPile.IsAmmoPile())
				{
					// Para munições, usar GetAmmoMax() (como Trader faz)
					int ammoMin = 1; // Munições sempre começam com mínimo 1
					int ammoMax = ammoPile.GetAmmoMax();
					
					// Log para debug
					Print("[AskalPurchase] 🔫 AMMO PILE detectado - Max: " + ammoMax + " | Solicitado: " + quantity);
					
					// Clamp usando limites de munição
					clampedQty = Math.Clamp(quantity, ammoMin, ammoMax);
					
					// Verificar quantidade inicial (antes de ServerSetAmmoCount)
					int initialAmmo = ammoPile.GetAmmoCount();
					Print("[AskalPurchase] 📦 STACKABLE (AmmoPile) - Quantidade inicial: " + initialAmmo + " | Solicitado: " + clampedQty + " | Max: " + ammoMax);
					
					// Munição stackable: usar ServerSetAmmoCount (como Trader e TraderX fazem)
					int ammoCount_stackable = Math.Round(clampedQty);
					ammoCount_stackable = Math.Clamp(ammoCount_stackable, ammoMin, ammoMax);
					
					ammoPile.ServerSetAmmoCount(ammoCount_stackable);
					ammoPile.SetSynchDirty(); // Sincronizar com cliente (como Trader faz)
					
					// Verificar quantidade aplicada
					int actualAmmo_stackable = ammoPile.GetAmmoCount();
					Print("[AskalPurchase] AMMO_SPAWN qty=" + actualAmmo_stackable + " type=" + itemClass + " player=" + player.GetIdentity().GetPlainId());
					
					if (actualAmmo_stackable != ammoCount_stackable)
					{
						Print("[AskalPurchase] ⚠️ Quantidade de munição não corresponde! Solicitado: " + ammoCount_stackable + " | Atual: " + actualAmmo_stackable);
						// Tentar novamente
						ammoPile.ServerSetAmmoCount(ammoCount_stackable);
						ammoPile.SetSynchDirty();
						actualAmmo_stackable = ammoPile.GetAmmoCount();
						
						if (actualAmmo_stackable != ammoCount_stackable)
						{
							Print("[AskalPurchase] ❌ ERRO: Não foi possível definir quantidade de munição! Solicitado: " + ammoCount_stackable + " | Final: " + actualAmmo_stackable);
						}
						else
						{
							Print("[AskalPurchase] 📦 STACKABLE (AmmoPile): " + ammoCount_stackable + " unidades aplicadas com sucesso");
						}
					}
					else
					{
						Print("[AskalPurchase] 📦 STACKABLE (AmmoPile): " + ammoCount_stackable + " unidades aplicadas com sucesso");
					}
				}
				else
				{
					// Item stackable não-ammo: usar SetQuantity normalmente
					float qtyMin = itemBase.GetQuantityMin();
					float qtyMax = itemBase.GetQuantityMax();
					clampedQty = Math.Clamp(quantity, qtyMin, qtyMax);
					
					// Verificar quantidade inicial (antes de SetQuantity)
					float initialQty = itemBase.GetQuantity();
					Print("[AskalPurchase] 📦 STACKABLE (não-ammo) - Quantidade inicial: " + initialQty + " | Solicitado: " + clampedQty + " | Max: " + qtyMax);
					
					itemBase.SetQuantity(clampedQty);
					
					// Verificar quantidade aplicada (depois de SetQuantity)
					float actualQty = itemBase.GetQuantity();
					Print("[AskalPurchase] AMMO_SPAWN qty=" + Math.Round(actualQty) + " type=" + itemClass + " player=" + player.GetIdentity().GetPlainId());
					
					// Se ainda não corresponde, tentar múltiplas vezes
					int retryCount = 0;
					while (Math.AbsFloat(actualQty - clampedQty) > 0.01 && retryCount < 3)
					{
						Print("[AskalPurchase] ⚠️ Quantidade não corresponde! Solicitado: " + clampedQty + " | Atual: " + actualQty + " | Tentativa " + (retryCount + 1));
						itemBase.SetQuantity(clampedQty);
						actualQty = itemBase.GetQuantity();
						retryCount++;
					}
					
					if (Math.AbsFloat(actualQty - clampedQty) > 0.01)
					{
						Print("[AskalPurchase] ❌ ERRO: Não foi possível definir quantidade! Solicitado: " + clampedQty + " | Final: " + actualQty);
					}
					else
					{
						Print("[AskalPurchase] 📦 STACKABLE: " + clampedQty + " unidades aplicadas com sucesso (min: " + qtyMin + ", max: " + qtyMax + ")");
					}
				}
			}
			else if (quantityType == 3 && itemBase.HasQuantity()) // QUANTIFIABLE
			{
				if (itemBase.IsLiquidContainer() && contentType > 0)
				{
					itemBase.SetLiquidType(contentType);
					float maxCapacity = itemBase.GetQuantityMax();
					float liquidAmount = maxCapacity * (quantity / 100.0);
					float clampedAmount = Math.Clamp(liquidAmount, itemBase.GetQuantityMin(), maxCapacity);
					itemBase.SetQuantity(clampedAmount);
					Print("[AskalPurchase] 💧 Líquido: " + clampedAmount + "mL");
				}
				else if (itemBase.HasQuantity())
				{
					// Para QUANTIFIABLE não-líquidos (comidas, carnes, etc), quantity vem como percentual (0-100)
					// Converter para quantidade real: maxQty * (percentual / 100)
					float maxQty = itemBase.GetQuantityMax();
					float qtyMin_quantifiable = itemBase.GetQuantityMin();
					
					// Clamp percentual entre 0-100
					float clampedPercent = Math.Clamp(quantity, 0.0, 100.0);
					
					// Converter percentual para quantidade real
					float actualQty_quantifiable = maxQty * (clampedPercent / 100.0);
					
					// Garantir que está dentro dos limites do item
					actualQty_quantifiable = Math.Clamp(actualQty_quantifiable, qtyMin_quantifiable, maxQty);
					
					itemBase.SetQuantity(actualQty_quantifiable);
					Print("[AskalPurchase] 📊 QUANTIFIABLE - Percentual: " + clampedPercent + "% | Quantidade real: " + actualQty_quantifiable + " (max: " + maxQty + ")");
				}
			}
			else
			{
				// Item não suporta quantidade variável - verificar se tem quantidade padrão
				if (itemBase.HasQuantity())
				{
					float defaultQty = itemBase.GetQuantity();
					Print("[AskalPurchase] 📦 Item não stackable - quantidade padrão: " + defaultQty);
				}
			}
		}

		return createdItem;
	}


	// Obter PlayerBase de PlayerIdentity (usando helper compartilhado)
	static PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
	{
		return AskalMarketHelpers.GetPlayerFromIdentity(identity);
	}
	
	// ETAPA 1: Buscar attachment slots compatíveis recursivamente
	// Busca desde os slots do player (camisa, calça, mochila, etc) até itens no inventário
	// Tenta encaixar o item comprado em qualquer slot de attachment disponível
	static EntityAI FindCompatibleAttachmentSlotRecursive(PlayerBase player, string itemClass)
	{
		if (!player || !itemClass || itemClass == "")
			return NULL;
		
		Print("[AskalPurchase] 🔍 Buscando attachment slots para: " + itemClass);
		
		// Buscar recursivamente em todos os itens que podem ter attachment slots
		// Começar pelo próprio player (slots de corpo: camisa, calça, mochila, etc)
		array<EntityAI> checkedItems = new array<EntityAI>();
		return FindCompatibleAttachmentSlotRecursiveInternal(player, player, itemClass, checkedItems);
	}
	
	// Função interna recursiva para buscar attachment slots
	// Tenta criar o item como attachment em cada item do inventário
	static EntityAI FindCompatibleAttachmentSlotRecursiveInternal(PlayerBase player, EntityAI parentItem, string itemClass, array<EntityAI> checkedItems)
	{
		if (!parentItem || !parentItem.GetInventory())
			return NULL;
		
		// Proteção contra loops infinitos
		if (checkedItems.Find(parentItem) != -1)
			return NULL;
		checkedItems.Insert(parentItem);
		
		// Tentar criar o item como attachment neste parent
		// CreateAttachment tenta automaticamente encontrar um slot compatível
		// Usar try-catch implícito: se falhar, apenas retorna NULL
		EntityAI attachedItem = NULL;
		if (parentItem.GetInventory())
		{
			attachedItem = parentItem.GetInventory().CreateAttachment(itemClass);
		}
		
		if (attachedItem)
		{
			Print("[AskalPurchase] ✅ Item encaixado em attachment slot de " + parentItem.GetType());
			return attachedItem;
		}
		
		// Se não encontrou, buscar recursivamente nos itens dentro deste parent
		array<EntityAI> childItems = new array<EntityAI>();
		if (parentItem.GetInventory())
		{
			parentItem.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, childItems);
		}
		
		foreach (EntityAI childItem : childItems)
		{
			if (!childItem || childItem == parentItem)
				continue;
			
			// Tentar encaixar neste child item
			if (childItem.GetInventory())
			{
				EntityAI foundItem = FindCompatibleAttachmentSlotRecursiveInternal(player, childItem, itemClass, checkedItems);
				if (foundItem)
					return foundItem;
			}
		}
		
		return NULL;
	}
	
	// ETAPA 2: Buscar inventory slots (cargo de containers) recursivamente
	static EntityAI FindInventorySlotRecursive(PlayerBase player, string itemClass)
	{
		if (!player || !itemClass || itemClass == "")
		{
			Print("[AskalPurchase] ⚠️ FindInventorySlotRecursive: Parâmetros inválidos");
			return NULL;
		}
		
		// Começar pelo inventário principal do player
		Print("[AskalPurchase] 🔍 Tentando criar no inventário principal...");
		if (player.GetInventory())
		{
			EntityAI createdItem = player.GetInventory().CreateInInventory(itemClass);
			if (createdItem)
			{
				Print("[AskalPurchase] ✅ Item criado no inventário principal");
				return createdItem;
			}
			else
			{
				Print("[AskalPurchase] ⚠️ Inventário principal cheio, buscando em containers...");
			}
		}
		else
		{
			Print("[AskalPurchase] ⚠️ player.GetInventory() retornou NULL");
		}
		
		// Buscar recursivamente em containers (mochilas, coletes, etc)
		array<EntityAI> checkedContainers = new array<EntityAI>();
		EntityAI foundItem = FindInventorySlotRecursiveInternal(player, player, itemClass, checkedContainers);
		if (foundItem)
		{
			Print("[AskalPurchase] ✅ Item encontrado em container recursivo");
		}
		else
		{
			Print("[AskalPurchase] ⚠️ Nenhum container com espaço encontrado");
		}
		return foundItem;
	}
	
	// Função interna recursiva para buscar inventory slots
	static EntityAI FindInventorySlotRecursiveInternal(PlayerBase player, EntityAI parentContainer, string itemClass, array<EntityAI> checkedContainers)
	{
		if (!parentContainer || !parentContainer.GetInventory())
			return NULL;
		
		// Proteção contra loops infinitos
		if (checkedContainers.Find(parentContainer) != -1)
			return NULL;
		checkedContainers.Insert(parentContainer);
		
		// Tentar criar no cargo deste container
		Print("[AskalPurchase] 🔍 Tentando criar em container: " + parentContainer.GetType());
		EntityAI createdItem = parentContainer.GetInventory().CreateInInventory(itemClass);
		if (createdItem)
		{
			Print("[AskalPurchase] ✅ Item criado em container: " + parentContainer.GetType());
			return createdItem;
		}
		
		// Se não encontrou, buscar recursivamente nos containers dentro deste
		array<EntityAI> childItems = new array<EntityAI>();
		if (parentContainer.GetInventory())
		{
			parentContainer.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, childItems);
		}
		
		Print("[AskalPurchase] 🔍 Verificando " + childItems.Count() + " itens dentro do container...");
		foreach (EntityAI childItem : childItems)
		{
			if (!childItem || childItem == parentContainer)
				continue;
			
			// Verificar se este child é um container
			if (childItem.GetInventory())
			{
				EntityAI foundItem = FindInventorySlotRecursiveInternal(player, childItem, itemClass, checkedContainers);
				if (foundItem)
					return foundItem;
			}
		}
		
		return NULL;
	}
	
		// Validar quantidade solicitada contra limites do item (validação servidor)
	static bool ValidateQuantity(string itemClass, float requestedQuantity, int quantityType)
	{
		if (!itemClass || itemClass == "")
			return false;
		
		// CRÍTICO: Para itens NONE (quantityType=0), aceitar quantity=-1 como válido
		// Itens não stackable não têm quantidade variável, então -1 é o valor padrão
		if (quantityType == 0) // NONE
		{
			// Para itens NONE, aceitar qualquer quantidade (incluindo -1)
			// A quantidade será ignorada na criação do item
			Print("[AskalPurchase] ✅ Item NONE (não stackable) - quantidade ignorada: " + requestedQuantity);
			return true;
		}
		
		// Para outros tipos, validar que quantidade > 0
		if (requestedQuantity <= 0)
		{
			Print("[AskalPurchase] ❌ Quantidade inválida (<= 0): " + requestedQuantity + " para tipo: " + quantityType);
			return false;
		}
		
		// Criar objeto temporário para validar limites
		Object tempObj = GetGame().CreateObject(itemClass, vector.Zero, true, false, false);
		if (!tempObj)
		{
			Print("[AskalPurchase] ⚠️ Não foi possível criar objeto temporário para validação: " + itemClass);
			return true; // Permitir se não conseguir validar (fallback permissivo)
		}
		
		ItemBase item = ItemBase.Cast(tempObj);
		if (!item)
		{
			GetGame().ObjectDelete(tempObj);
			return true; // Não é ItemBase, permitir (validação será feita em CreateSimpleItem)
		}
		
		bool isValid = false;
		
		// PRIORIDADE 1: Verificar se é Magazine primeiro (mesmo que quantityType seja 2, pode ser ammo stackable)
		Magazine mag = Magazine.Cast(item);
		if (mag && quantityType == 1) // MAGAZINE
		{
			int ammoMax = mag.GetAmmoMax();
			isValid = (requestedQuantity >= 0 && requestedQuantity <= ammoMax);
			if (!isValid)
				Print("[AskalPurchase] ❌ Quantidade inválida para magazine: " + requestedQuantity + " (max: " + ammoMax + ")");
		}
		else if (quantityType == 2) // STACKABLE
		{
			Print("[AskalPurchase] 🔍 Validando STACKABLE: " + itemClass + " | Qty solicitada: " + requestedQuantity);
			
			// Para ammo, priorizar config sobre HasQuantity (config é mais confiável)
			if (itemClass.IndexOf("Ammo_") == 0 || itemClass.IndexOf("ammo_") == 0)
			{
				string configPath = "CfgMagazines " + itemClass + " count";
				if (GetGame().ConfigIsExisting(configPath))
				{
					int configCount = GetGame().ConfigGetInt(configPath);
					isValid = (requestedQuantity >= 1 && requestedQuantity <= configCount);
					if (isValid)
					{
						Print("[AskalPurchase] ✅ Ammo validado pelo config: " + requestedQuantity + " (max: " + configCount + ")");
					}
					else
					{
						Print("[AskalPurchase] ❌ Quantidade inválida para ammo (config): " + requestedQuantity + " (max: " + configCount + ")");
					}
				}
				else if (item.HasQuantity())
				{
					// Se não tem config, usar HasQuantity como fallback
					float qtyMin_ammo = item.GetQuantityMin();
					float qtyMax_ammo = item.GetQuantityMax();
					isValid = (requestedQuantity >= qtyMin_ammo && requestedQuantity <= qtyMax_ammo);
					if (isValid)
					{
						Print("[AskalPurchase] ✅ Ammo validado por HasQuantity: " + requestedQuantity + " (min: " + qtyMin_ammo + ", max: " + qtyMax_ammo + ")");
					}
					else
					{
						Print("[AskalPurchase] ❌ Quantidade inválida para ammo (HasQuantity): " + requestedQuantity + " (min: " + qtyMin_ammo + ", max: " + qtyMax_ammo + ")");
					}
				}
				else
				{
					// Fallback permissivo para ammo sem config e sem HasQuantity
					Print("[AskalPurchase] ⚠️ Ammo sem config e sem HasQuantity - permitindo: " + requestedQuantity);
					isValid = true;
				}
			}
			else if (item.HasQuantity())
			{
				// Para stackables não-ammo, usar HasQuantity
				float qtyMin = item.GetQuantityMin();
				float qtyMax = item.GetQuantityMax();
				isValid = (requestedQuantity >= qtyMin && requestedQuantity <= qtyMax);
				if (isValid)
				{
					Print("[AskalPurchase] ✅ Stackable validado: " + requestedQuantity + " (min: " + qtyMin + ", max: " + qtyMax + ")");
				}
				else
				{
					Print("[AskalPurchase] ❌ Quantidade inválida para stackable: " + requestedQuantity + " (min: " + qtyMin + ", max: " + qtyMax + ")");
				}
			}
			else
			{
				// Não é ammo e não tem HasQuantity - permitir quantidade padrão
				Print("[AskalPurchase] ⚠️ Stackable sem HasQuantity - permitindo quantidade padrão: " + requestedQuantity);
				isValid = (requestedQuantity == 1);
			}
		}
		else if (quantityType == 3 && item.HasQuantity()) // QUANTIFIABLE
		{
			// Para QUANTIFIABLE, requestedQuantity é um PERCENTUAL (0-100), não quantidade absoluta
			// Validar apenas se está no range de percentual válido
			isValid = (requestedQuantity >= 0.0 && requestedQuantity <= 100.0);
			if (!isValid)
				Print("[AskalPurchase] ❌ Percentual inválido para quantifiable: " + requestedQuantity + " (deve estar entre 0-100)");
			else
				Print("[AskalPurchase] ✅ Percentual válido para quantifiable: " + requestedQuantity + "%");
		}
		else if (quantityType == 3) // QUANTIFIABLE sem HasQuantity (permitir, será tratado em CreateSimpleItem)
		{
			// Para itens QUANTIFIABLE sem HasQuantity, validar apenas o percentual
			isValid = (requestedQuantity >= 0.0 && requestedQuantity <= 100.0);
			if (!isValid)
				Print("[AskalPurchase] ❌ Percentual inválido para quantifiable (sem HasQuantity): " + requestedQuantity);
			else
				Print("[AskalPurchase] ✅ Percentual válido para quantifiable (sem HasQuantity): " + requestedQuantity + "%");
		}
		else
		{
			// Tipo não suporta quantidade ou quantidade padrão (1)
			isValid = (requestedQuantity == 1 || requestedQuantity == 0);
		}
		
		GetGame().ObjectDelete(tempObj);
		return isValid;
	}

	// Calcular preço total considerando quantidade (para stackables)
	static int ComputeItemTotalPriceWithQuantity(string itemClass, float quantity, int quantityType)
	{
		ItemData itemData = AskalDatabase.GetItem(itemClass);
		if (!itemData)
			return -1;

		int basePrice = NormalizeBuyPrice(itemData.Price);

		// Adicionar preço dos attachments
		array<string> attachments = itemData.GetAttachments();
		if (attachments)
		{
			for (int i = 0; i < attachments.Count(); i++)
			{
				string attachmentClass = attachments.Get(i);
				if (!attachmentClass || attachmentClass == "")
					continue;

				ItemData attachmentData = AskalDatabase.GetItem(attachmentClass);
				if (attachmentData)
					basePrice += NormalizeBuyPrice(attachmentData.Price);
			}
		}

		int unitPrice = ApplyBuyCoefficient(basePrice);

		// Para stackables, multiplicar pelo quantidade
		if (quantityType == 2) // STACKABLE
		{
			float totalPriceFloat = unitPrice * quantity;
			int totalPrice = Math.Round(totalPriceFloat);
			if (totalPrice <= 0)
				totalPrice = 1;

			Print("[AskalPurchase] [STACKABLE] Preço: " + unitPrice + " x " + quantity + " = " + totalPrice);
			return totalPrice;
		}

		return unitPrice;
	}

	protected static int NormalizeBuyPrice(int price)
	{
		if (price <= 0)
			price = AskalMarketDefaults.DEFAULT_BUY_PRICE;
		return price;
	}

	protected static int ApplyBuyCoefficient(int price)
	{
		price = NormalizeBuyPrice(price);
		float coeff = AskalVirtualStoreSettings.GetBuyCoefficient();
		float adjustedFloat = price * coeff;
		int adjusted = Math.Round(adjustedFloat);
		if (adjusted <= 0)
			adjusted = 1;
		return adjusted;
	}

	// Criar attachments recursivamente (com limite de profundidade)
	// depth: profundidade atual (0 = item raiz)
	static void AttachDefaultAttachmentsRecursive(EntityAI parentEntity, string parentClass, int depth, string playerId)
	{
		if (!parentEntity || !parentClass || parentClass == "")
			return;
		
		// Safety guard: evitar recursão infinita
		if (depth >= 4)
		{
			Print("[AskalPurchase] ⚠️ Profundidade máxima de attachments atingida (4) para: " + parentClass);
			return;
		}
		
		// Ler attachments do dataset/config
		array<string> attachments = GetAttachmentsForItem(parentClass);
		if (!attachments || attachments.Count() == 0)
			return;
		
		for (int i = 0; i < attachments.Count(); i++)
		{
			string attachmentClass = attachments.Get(i);
			if (!attachmentClass || attachmentClass == "")
				continue;
			
			// Criar attachment no parent
			EntityAI attachmentEntity = EntityAI.Cast(parentEntity.GetInventory().CreateAttachment(attachmentClass));
			if (!attachmentEntity)
			{
				Print("[AskalPurchase] ⚠️ Falha ao anexar attachment: " + attachmentClass + " ao parent: " + parentClass);
				continue;
			}
			
			// Log criação do attachment
			Print("[AskalPurchase] ATTACH_CREATED parent=" + parentClass + " att=" + attachmentClass + " player=" + playerId + " depth=" + depth);
			
			// Recursivamente criar attachments para este attachment
			AttachDefaultAttachmentsRecursive(attachmentEntity, attachmentClass, depth + 1, playerId);
		}
	}
	
	// Obter attachments para um item (do dataset ou config)
	static array<string> GetAttachmentsForItem(string itemClass)
	{
		array<string> attachments = new array<string>();
		
		// 1. Tentar ler do dataset (ItemData)
		ItemData itemData = AskalDatabase.GetItem(itemClass);
		if (itemData)
		{
			array<string> datasetAttachments = itemData.GetAttachments();
			if (datasetAttachments && datasetAttachments.Count() > 0)
			{
				for (int i = 0; i < datasetAttachments.Count(); i++)
				{
					string att = datasetAttachments.Get(i);
					if (att && att != "")
						attachments.Insert(att);
				}
				return attachments; // Retornar se encontrou no dataset
			}
		}
		
		// 2. Tentar ler do config (CfgVehicles, CfgWeapons, CfgMagazines)
		TStringArray configs = new TStringArray();
		configs.Insert("CfgVehicles");
		configs.Insert("CfgWeapons");
		configs.Insert("CfgMagazines");
		
		TStringArray foundAttachments = new TStringArray();
		foreach (string configPath : configs)
		{
			string fullPath = configPath + " " + itemClass + " attachments";
			GetGame().ConfigGetTextArray(fullPath, foundAttachments);
			
			if (foundAttachments.Count() > 0)
			{
				for (int j = 0; j < foundAttachments.Count(); j++)
				{
					string att_local = foundAttachments.Get(j);
					if (att_local && att_local != "")
						attachments.Insert(att_local);
				}
				break; // Encontrou no primeiro config válido
			}
		}
		
		return attachments;
	}
	
	// Wrapper público (mantém compatibilidade com código existente)
	protected static void AttachDefaultAttachments(EntityAI itemEntity, string itemClass)
	{
		if (!itemEntity || !itemClass || itemClass == "")
			return;
		
		// Obter player ID para logging (se disponível)
		string playerId = "";
		PlayerBase player = PlayerBase.Cast(itemEntity.GetHierarchyRootPlayer());
		if (player && player.GetIdentity())
		{
			playerId = player.GetIdentity().GetPlainId();
			if (!playerId || playerId == "")
				playerId = player.GetIdentity().GetId();
		}
		
		// Criar attachments recursivamente (inicia em depth=0)
		AttachDefaultAttachmentsRecursive(itemEntity, itemClass, 0, playerId);
	}
}

