	// ==========================================
// AskalPurchaseService - Serviço de compra (4_World - acesso a PlayerBase)
// ==========================================

class AskalPurchaseService
{
	// Processar compra COM quantidade e conteúdo customizados
	static bool ProcessPurchaseWithQuantity(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId, float itemQuantity, int quantityType, int contentType)
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
		
		// Criar item
		EntityAI createdItem = CreateSimpleItem(player, itemClass, itemQuantity, quantityType, contentType);
		if (!createdItem)
		{
			Print("[AskalPurchase] ❌ Não foi possível criar item: " + itemClass);
			// ITER-1: Release reservation if item creation fails
			AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
			return false;
		}

		// Attachments padrão (apenas se item foi criado com sucesso)
		AttachDefaultAttachments(createdItem, itemClass);

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

	// Criar item simples (limpo e direto)
	// Busca recursivamente por espaço no inventário (incluindo containers) e sempre dropa no chão se necessário
	static EntityAI CreateSimpleItem(PlayerBase player, string itemClass, float quantity, int quantityType, int contentType)
	{
		Print("[AskalPurchase] 🛠️ Criando item: " + itemClass);

		// ESTRATÉGIA 1: Tentar criar no inventário principal (busca recursiva automática)
		EntityAI item = player.GetInventory().CreateInInventory(itemClass);
		if (item)
		{
			Print("[AskalPurchase] ✅ Item criado no inventário principal");
		}
		else
		{
			Print("[AskalPurchase] ⚠️ Inventário principal cheio, tentando nas mãos...");
			// ESTRATÉGIA 2: Tentar criar nas mãos
			item = player.GetHumanInventory().CreateInHands(itemClass);
			if (item)
			{
				Print("[AskalPurchase] ✅ Item criado nas mãos");
			}
			else
			{
				Print("[AskalPurchase] ⚠️ Mãos ocupadas, tentando buscar espaço em containers...");
				// ESTRATÉGIA 3: Buscar recursivamente em containers (mochilas, etc)
				item = FindSpaceInContainers(player, itemClass);
				if (item)
				{
					Print("[AskalPurchase] ✅ Item criado em container");
				}
			}
		}

		// ESTRATÉGIA 4: Se ainda não encontrou espaço, dropar no chão
		if (!item)
		{
			Print("[AskalPurchase] ⚠️ Nenhum espaço encontrado, criando no chão...");
			vector playerPos = player.GetPosition();
			// Adicionar pequeno offset para evitar que o item fique dentro do player
			playerPos[1] = playerPos[1] + 0.5; // Elevar um pouco
			Object itemObj = GetGame().CreateObjectEx(itemClass, playerPos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS);
			item = EntityAI.Cast(itemObj);
			
			if (!item)
			{
				Print("[AskalPurchase] ❌ Falha crítica ao criar item no chão: " + itemClass);
				return NULL;
			}
			
			Print("[AskalPurchase] ⚠️ Item criado no chão próximo ao player: " + itemClass);
		}

		// Aplicar quantidade apenas para tipos que funcionam
		ItemBase itemBase = ItemBase.Cast(item);
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
				// Para stackables, definir quantidade diretamente
				float qtyMin = itemBase.GetQuantityMin();
				float qtyMax = itemBase.GetQuantityMax();
				float clampedQty = Math.Clamp(quantity, qtyMin, qtyMax);
				
				// Verificar quantidade inicial (antes de SetQuantity)
				float initialQty = itemBase.GetQuantity();
				Print("[AskalPurchase] 📦 STACKABLE - Quantidade inicial: " + initialQty + " | Solicitado: " + clampedQty + " | Max: " + qtyMax);
				
				// CRÍTICO: Forçar quantidade mesmo se já foi criado com valor padrão
				// Alguns itens são criados com quantidade padrão (metade do max), precisamos sobrescrever
				itemBase.SetQuantity(clampedQty);
				
				// Aguardar um frame para garantir que SetQuantity foi aplicado
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

		return item;
	}


	// Obter PlayerBase de PlayerIdentity (usando helper compartilhado)
	static PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
	{
		return AskalMarketHelpers.GetPlayerFromIdentity(identity);
	}
	
	// Buscar espaço recursivamente em containers (mochilas, etc)
	static EntityAI FindSpaceInContainers(PlayerBase player, string itemClass)
	{
		if (!player || !itemClass || itemClass == "")
			return NULL;
		
		// Enumerar todos os itens no inventário do player
		array<EntityAI> allItems = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, allItems);
		
		// Procurar containers (mochilas, etc) e tentar criar o item dentro deles
		for (int i = 0; i < allItems.Count(); i++)
		{
			EntityAI containerItem = allItems.Get(i);
			if (!containerItem || !containerItem.GetInventory())
				continue;
			
			// Tentar criar o item dentro deste container
			EntityAI item = containerItem.GetInventory().CreateInInventory(itemClass);
			if (item)
			{
				Print("[AskalPurchase] ✅ Item criado em container: " + containerItem.GetType());
				return item;
			}
		}
		
		return NULL;
	}
	
		// Validar quantidade solicitada contra limites do item (validação servidor)
	static bool ValidateQuantity(string itemClass, float requestedQuantity, int quantityType)
	{
		if (!itemClass || itemClass == "")
			return false;
		
		if (requestedQuantity <= 0)
			return false;
		
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
			// Para stackables, verificar GetQuantityMax() primeiro
			if (item.HasQuantity())
			{
				float qtyMin = item.GetQuantityMin();
				float qtyMax = item.GetQuantityMax();
				isValid = (requestedQuantity >= qtyMin && requestedQuantity <= qtyMax);
				if (!isValid)
					Print("[AskalPurchase] ❌ Quantidade inválida para stackable: " + requestedQuantity + " (min: " + qtyMin + ", max: " + qtyMax + ")");
			}
			else
			{
				// Se não tem HasQuantity(), tentar ler do config (para ammo que pode não ter HasQuantity mas tem count no config)
				if (itemClass.IndexOf("Ammo_") == 0 || itemClass.IndexOf("ammo_") == 0)
				{
					string configPath = "CfgMagazines " + itemClass + " count";
					if (GetGame().ConfigIsExisting(configPath))
					{
						int configCount = GetGame().ConfigGetInt(configPath);
						isValid = (requestedQuantity >= 1 && requestedQuantity <= configCount);
						if (!isValid)
							Print("[AskalPurchase] ❌ Quantidade inválida para ammo (config): " + requestedQuantity + " (max: " + configCount + ")");
					}
					else
					{
						// Fallback permissivo para ammo sem config
						Print("[AskalPurchase] ⚠️ Ammo sem HasQuantity() e sem config - permitindo: " + requestedQuantity);
						isValid = true;
					}
				}
				else
				{
					// Não é ammo e não tem HasQuantity - permitir quantidade padrão
					isValid = (requestedQuantity == 1);
				}
			}
		}
		else if (quantityType == 3 && item.HasQuantity()) // QUANTIFIABLE
		{
			float qtyMin_local = item.GetQuantityMin();
			float qtyMax_local = item.GetQuantityMax();
			isValid = (requestedQuantity >= qtyMin_local && requestedQuantity <= qtyMax_local);
			if (!isValid)
				Print("[AskalPurchase] ❌ Quantidade inválida para quantifiable: " + requestedQuantity + " (min: " + qtyMin_local + ", max: " + qtyMax_local + ")");
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

