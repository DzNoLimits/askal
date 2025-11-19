// ==========================================
// AskalSellService - Serviço de venda de itens (ULTRA SIMPLIFICADO)
// ==========================================

class AskalSellService
{
	// Processar venda de item - VERSÃO ULTRA SIMPLIFICADA
	// 1. Validar se item pode ser removido
	// 2. Buscar valor no database
	// 3. Calcular preço
	// 4. Adicionar dinheiro
	// 5. Remover item
	static bool ProcessSell(PlayerIdentity identity, string steamId, EntityAI itemToSell, string currencyId, int transactionMode, out int outPrice)
	{
		Print("[AskalSell] [PROCESSO] Iniciando ProcessSell");
		
		if (!identity || !itemToSell)
		{
			Print("[AskalSell] [ERRO] Identity ou item invalido");
			return false;
		}
		
		PlayerBase player = GetPlayerFromIdentity(identity);
		if (!player)
		{
			Print("[AskalSell] [ERRO] Player nao encontrado");
			return false;
		}
		
		string itemClass = itemToSell.GetType();
		Print("[AskalSell] [PROCESSO] Item: " + itemClass);
		
		// ÚNICA VALIDAÇÃO: Item pode ser removido?
		if (!itemToSell.GetInventory())
		{
			Print("[AskalSell] [ERRO] Item nao tem inventory");
			return false;
		}
		
		if (!itemToSell.GetInventory().CanRemoveEntity())
		{
			Print("[AskalSell] [ERRO] Item nao pode ser removido (CanRemoveEntity = false)");
			return false;
		}
		Print("[AskalSell] [VALIDACAO] Item pode ser removido");
		
		// Busca item no database (case-insensitive)
		Print("[AskalSell] [DATABASE] Buscando item no database: " + itemClass);
		ItemData serverItemData = AskalDatabase.GetItemCaseInsensitive(itemClass);
		if (!serverItemData)
		{
			Print("[AskalSell] [ERRO] Item nao encontrado no database: " + itemClass);
			return false;
		}
		Print("[AskalSell] [DATABASE] Item encontrado - Price: " + serverItemData.Price + " | SellPercent: " + serverItemData.SellPercent);
		
		// Calcula preço PROPORCIONAL À INTEGRIDADE
		int basePrice = serverItemData.Price;
		if (basePrice <= 0)
			basePrice = AskalMarketDefaults.DEFAULT_BUY_PRICE;
		
		int sellPercent = serverItemData.SellPercent;
		if (sellPercent <= 0)
			sellPercent = AskalMarketDefaults.DEFAULT_SELL_PERCENT;
		
		// Obter integridade do item (0.0 a 1.0, onde 1.0 = 100%)
		float health01 = itemToSell.GetHealth01();
		float healthPercent = health01 * 100.0;
		
		// Garantir que health está entre 0 e 100
		if (healthPercent < 0)
			healthPercent = 0;
		if (healthPercent > 100)
			healthPercent = 100;
		
		Print("[AskalSell] [HEALTH] Integridade do item: " + healthPercent + "% (health01: " + health01 + ")");
		
		// Obter coeficiente de venda (uma vez só)
			float sellCoeff = AskalVirtualStoreSettings.GetSellCoefficient();
		
		// Calcular preço base: basePrice * sellPercent
		float baseSellPrice = basePrice * (sellPercent / 100.0);
		
		// Aplicar proporção de integridade: preço proporcional à health (1% health = 1% do valor)
		float priceWithHealth = baseSellPrice * (healthPercent / 100.0);
		
		// CALCULAR PREÇO DE MUNIÇÃO EM CARREGADORES
		float ammoPrice = 0.0;
		Magazine mag = Magazine.Cast(itemToSell);
		if (mag)
		{
			int ammoCount = mag.GetAmmoCount();
			if (ammoCount > 0)
			{
				// Obter tipo de munição do carregador via config
				string magazineClass = itemToSell.GetType();
				string ammoType = "";
				GetGame().ConfigGetText("CfgMagazines " + magazineClass + " ammo", ammoType);
				
				if (ammoType && ammoType != "")
				{
					// Tentar encontrar item de munição no database
					// Formato pode ser "Bullet_XXX" ou "Ammo_XXX"
					string ammoItemClass = "";
					if (ammoType.IndexOf("Bullet_") == 0)
					{
						// Converter "Bullet_XXX" para "Ammo_XXX"
						ammoItemClass = "Ammo_" + ammoType.Substring(7, ammoType.Length() - 7);
					}
					else if (ammoType.IndexOf("Ammo_") == 0)
					{
						ammoItemClass = ammoType;
					}
					
					if (ammoItemClass != "")
					{
						// Buscar preço da munição no database
						ItemData ammoData = AskalDatabase.GetItemCaseInsensitive(ammoItemClass);
						if (ammoData && ammoData.Price > 0)
						{
							// Calcular preço por unidade de munição
							// Ler stack máximo do config para calcular preço unitário
							float ammoUnitPrice = ammoData.Price;
							int ammoMaxQty = 1;
							
							string configPath = "CfgMagazines " + ammoItemClass + " count";
							if (GetGame().ConfigIsExisting(configPath))
							{
								ammoMaxQty = GetGame().ConfigGetInt(configPath);
								if (ammoMaxQty > 0)
									ammoUnitPrice = ammoData.Price / ammoMaxQty;
							}
							
							// Calcular preço total da munição: quantidade * preço unitário * sellPercent * health
							float ammoBasePrice = ammoCount * ammoUnitPrice;
							float ammoSellPrice = ammoBasePrice * (sellPercent / 100.0) * (healthPercent / 100.0);
							
							// Aplicar coeficiente de venda
							if (sellCoeff > 0)
								ammoSellPrice = ammoSellPrice * sellCoeff;
							
							ammoPrice = ammoSellPrice;
							Print("[AskalSell] [MUNICAO] Carregador com " + ammoCount + "x " + ammoItemClass + " = $" + Math.Round(ammoPrice));
						}
					}
				}
			}
		}
		
		// CALCULAR PREÇO DE QUANTIDADE EM STACKABLES
		float quantityPrice = 0.0;
		ItemBase itemBase = ItemBase.Cast(itemToSell);
		if (itemBase && itemBase.HasQuantity() && !mag) // Não processar se já é magazine
		{
			float currentQty = itemBase.GetQuantity();
			float maxQty = itemBase.GetQuantityMax();
			
			if (maxQty > 0 && currentQty > 0)
			{
				// CORREÇÃO: Calcular preço unitário primeiro
				// basePrice é o preço de uma pilha completa (maxQty unidades)
				// Preço unitário = basePrice / maxQty
				float unitPrice = basePrice / maxQty;
				
				// Preço base de venda por unidade = unitPrice * sellPercent
				float unitSellPrice = unitPrice * (sellPercent / 100.0);
				
				// Preço total = quantidade atual * preço unitário de venda * health * sellCoeff
				float totalUnitPrice = currentQty * unitSellPrice;
				quantityPrice = totalUnitPrice * (healthPercent / 100.0);
				
				// Aplicar coeficiente de venda
				if (sellCoeff > 0)
					quantityPrice = quantityPrice * sellCoeff;
				
				Print("[AskalSell] [QUANTIDADE] Item stackable: " + currentQty + "/" + maxQty + " unidades | Preço unitário: " + Math.Round(unitPrice) + " | Preço total: $" + Math.Round(quantityPrice));
			}
		}
		
		// Se tem quantityPrice, usar ele em vez de priceWithHealth (para stackables)
		if (quantityPrice > 0)
			priceWithHealth = quantityPrice;
		else
		{
			// Aplicar coeficiente de venda no preço do item (se não é stackable)
			if (sellCoeff > 0)
				priceWithHealth = priceWithHealth * sellCoeff;
		}
		
		// Soma total: preço do item + preço da munição
		float totalPriceFloat = priceWithHealth + ammoPrice;
		
		int totalPrice = Math.Round(totalPriceFloat);
		
		// Garantir mínimo de 1 (mesmo com 1% health)
		if (totalPrice <= 0)
			totalPrice = 1;
		
		Print("[AskalSell] [PRECO] Preco calculado: " + totalPrice + " (item: " + Math.Round(priceWithHealth) + ", municao: " + Math.Round(ammoPrice) + ", base: " + basePrice + ", sell%: " + sellPercent + ", health%: " + healthPercent + ", coeff: " + sellCoeff + ")");
		
		// VERIFICAR: Se item tem cargo, NÃO permitir venda (deve estar vazio) - ANTES de processar pagamento
		Print("[AskalSell] [VALIDACAO] Verificando se item tem cargo...");
		bool hasCargo = HasCargoItemsRecursive(itemToSell);
		Print("[AskalSell] [VALIDACAO] Resultado da verificação de cargo: " + hasCargo);
		if (hasCargo)
		{
			Print("[AskalSell] [ERRO] Item tem cargo - venda bloqueada");
			outPrice = 0;
			return false;
		}
		Print("[AskalSell] [VALIDACAO] ✅ Item não tem cargo - venda permitida");
		
		// Retornar preço via parâmetro de saída
		outPrice = totalPrice;
	
		// Adiciona dinheiro ANTES de remover item
		Print("[AskalSell] [PAGAMENTO] Adicionando dinheiro (Mode: " + transactionMode + ")...");
		bool paymentSuccess = false;
		
		if (transactionMode == 1)
		{
			paymentSuccess = AskalCurrencyInventoryManager.AddPhysicalCurrency(player, totalPrice, currencyId);
		}
		else if (transactionMode == 2)
		{
			paymentSuccess = AskalPlayerBalance.AddBalance(steamId, totalPrice, currencyId);
		}
		else
		{
			Print("[AskalSell] [ERRO] TransactionMode invalido: " + transactionMode);
		return false;
	}
	
		if (!paymentSuccess)
	{
			Print("[AskalSell] [ERRO] Falha ao adicionar dinheiro");
			outPrice = 0;
			return false;
		}
		Print("[AskalSell] [PAGAMENTO] Dinheiro adicionado com sucesso");
		
		// Gerar descrição detalhada da venda (inclui attachments)
		string sellDescription = BuildSellDescription(itemToSell, itemClass);
		
		// Remove item do inventário (attachments são vendidos junto)
		Print("[AskalSell] [REMOCAO] Removendo item...");
		GetGame().ObjectDelete(itemToSell);
		Print("[AskalSell] [REMOCAO] Item removido");
		
		// Notificar cliente com descrição detalhada (apenas no menu)
		AskalNotificationHelper.AddSellNotification(itemClass, totalPrice, sellDescription);
		
		Print("[AskalSell] [SUCESSO] Venda concluida: " + itemClass + " - " + totalPrice + " " + currencyId);
		return true;
	}
	
	// Obter PlayerBase de PlayerIdentity (usando helper compartilhado)
	static PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
	{
		return AskalMarketHelpers.GetPlayerFromIdentity(identity);
	}
	
	// Verificar recursivamente se item tem cargo (itens dentro de containers)
	// Versão interna com proteção contra loops infinitos
	static bool HasCargoItemsRecursiveInternal(EntityAI item, array<EntityAI> checkedItems)
	{
		if (!item || !item.GetInventory())
			return false; // Sem inventory = sem cargo
		
		// Proteção contra loops infinitos: verificar se já foi checado
		if (checkedItems.Find(item) != -1)
			return false; // Já foi verificado, evitar loop
		
		checkedItems.Insert(item);
		
		// Obter lista de attachments para excluí-los
		array<EntityAI> attachments = new array<EntityAI>();
		int attCount = item.GetInventory().AttachmentCount();
		for (int attIdx = 0; attIdx < attCount; attIdx++)
		{
			EntityAI attachment = item.GetInventory().GetAttachmentFromIndex(attIdx);
			if (attachment)
				attachments.Insert(attachment);
		}
		
		// Enumerar todos os itens no inventário
		array<EntityAI> allItems = new array<EntityAI>();
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, allItems);
		
		// Verificar cada item para ver se está no cargo (não é attachment e não é o próprio item)
		foreach (EntityAI cargoItem : allItems)
		{
			if (!cargoItem || cargoItem == item)
				continue;
			
			// Ignorar attachments (verificação dupla para garantir)
			if (attachments.Find(cargoItem) != -1)
				continue;
			
			// Verificar se o item está realmente no cargo deste container
			// Usar GetCurrentInventoryLocation para verificar o parent
			InventoryLocation itemLoc = new InventoryLocation();
			if (cargoItem.GetInventory() && cargoItem.GetInventory().GetCurrentInventoryLocation(itemLoc))
			{
				EntityAI parent = itemLoc.GetParent();
				// Se o parent é o item sendo verificado, então está no cargo
				if (parent == item)
				{
					// Verificar o tipo de slot - attachments geralmente têm slots negativos
					int slot = itemLoc.GetSlot();
					// Slots negativos são geralmente attachments ou slots especiais
					// Slots normais de cargo são >= 0
					// Mas alguns containers podem ter slots negativos válidos, então não confiar apenas nisso
					
					// Verificação adicional: se já foi identificado como attachment acima, não considerar
					// Se chegou aqui e não é attachment, então está no cargo
					
					// Item encontrado no cargo - verificar recursivamente se ele também tem cargo
					if (HasCargoItemsRecursiveInternal(cargoItem, checkedItems))
						return true; // Container dentro de container com itens
					
					// Item no cargo encontrado
					return true;
				}
			}
		}
		
		return false; // Sem itens no cargo
	}
	
	// Pública para ser chamada de AskalSellModule
	static bool HasCargoItemsRecursive(EntityAI item)
	{
		array<EntityAI> checkedItems = new array<EntityAI>();
		return HasCargoItemsRecursiveInternal(item, checkedItems);
	}
	
	// Gerar descrição detalhada da venda (inclui attachments)
	static string BuildSellDescription(EntityAI itemToSell, string itemClass)
	{
		string description = GetItemDisplayName(itemClass);
		
		// Verificar attachments
		if (itemToSell && itemToSell.GetInventory())
		{
			int attachmentCount = itemToSell.GetInventory().AttachmentCount();
			if (attachmentCount > 0)
			{
				array<string> attachmentNames = new array<string>();
				
				for (int i = 0; i < attachmentCount; i++)
				{
					EntityAI att = itemToSell.GetInventory().GetAttachmentFromIndex(i);
					if (att)
					{
						string attClass = att.GetType();
						string attName = GetItemDisplayName(attClass);
						attachmentNames.Insert(attName);
					}
				}
				
				// Formato: "Lanterna + Bateria 9V"
				for (int j = 0; j < attachmentNames.Count(); j++)
				{
					if (j == 0)
						description = description + " + ";
					else
						description = description + ", ";
					description = description + attachmentNames.Get(j);
				}
			}
		}
		
		return description;
	}
	
	// Obter display name do item (usando helper compartilhado)
	static string GetItemDisplayName(string className)
	{
		return AskalMarketHelpers.GetItemDisplayName(className);
	}
	
	// Esta função não é mais usada - validação de cargo agora bloqueia venda
	// Mantida apenas para referência
}
