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
							// Assumir que o preço do item Ammo_XXX é para 1 unidade
							// Se o item tem quantidade, dividir pelo máximo
							float ammoUnitPrice = ammoData.Price;
							
							// Verificar se é stackable e tem quantidade máxima
							ItemBase tempAmmo = ItemBase.Cast(GetGame().CreateObjectEx(ammoItemClass, vector.Zero, ECE_PLACE_ON_SURFACE, RF_DEFAULT));
							if (tempAmmo && tempAmmo.HasQuantity())
							{
								float ammoMaxQty = tempAmmo.GetQuantityMax();
								if (ammoMaxQty > 0)
									ammoUnitPrice = ammoData.Price / ammoMaxQty;
							}
							if (tempAmmo)
								GetGame().ObjectDelete(tempAmmo);
							
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
				// Calcular preço proporcional à quantidade
				float quantityPercent = (currentQty / maxQty) * 100.0;
				quantityPrice = baseSellPrice * (quantityPercent / 100.0);
				
				// Aplicar coeficiente de venda
				if (sellCoeff > 0)
					quantityPrice = quantityPrice * sellCoeff;
				
				Print("[AskalSell] [QUANTIDADE] Item com " + currentQty + "/" + maxQty + " (" + quantityPercent + "%) = $" + Math.Round(quantityPrice));
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
		
		// Retornar preço via parâmetro de saída
		outPrice = totalPrice;
		
		// VERIFICAR: Se item tem cargo, NÃO permitir venda (deve estar vazio) - ANTES de processar pagamento
		Print("[AskalSell] [VALIDACAO] Verificando se item tem cargo...");
		if (HasCargoItemsRecursive(itemToSell))
		{
			Print("[AskalSell] [ERRO] Item tem cargo - venda bloqueada");
			outPrice = 0;
		return false;
	}
	
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
	
	// Obter PlayerBase de PlayerIdentity
	static PlayerBase GetPlayerFromIdentity(PlayerIdentity identity)
	{
		if (!identity)
			return NULL;
		
		array<Man> players = new array<Man>;
		GetGame().GetWorld().GetPlayerList(players);
		
		string senderId = identity.GetId();
		for (int playerIdx = 0; playerIdx < players.Count(); playerIdx++)
		{
			PlayerBase player = PlayerBase.Cast(players.Get(playerIdx));
			if (!player)
				continue;
			
			PlayerIdentity playerIdentity = player.GetIdentity();
			if (playerIdentity && playerIdentity.GetId() == senderId)
				return player;
		}
		
		return NULL;
	}
	
	// Verificar recursivamente se item tem cargo (itens dentro de containers)
	// Versão interna com proteção contra loops infinitos
	static bool HasCargoItemsRecursiveInternal(EntityAI item, array<EntityAI> checkedItems)
	{
		if (!item || !item.GetInventory())
			return false;
		
		// Proteção contra loops infinitos: verificar se já foi checado
		if (checkedItems.Find(item) != -1)
			return false; // Já foi verificado, evitar loop
		
		checkedItems.Insert(item);
		
		// Primeiro, verificar attachments para excluí-los
		array<EntityAI> attachments = new array<EntityAI>();
		int attCount = item.GetInventory().AttachmentCount();
		for (int attIdx = 0; attIdx < attCount; attIdx++)
		{
			EntityAI attachment = item.GetInventory().GetAttachmentFromIndex(attIdx);
			if (attachment)
				attachments.Insert(attachment);
		}
		
		// Agora enumerar todos os itens
		array<EntityAI> allItems = new array<EntityAI>();
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, allItems);
		
		// Filtrar apenas itens que estão no cargo (não attachments, não o próprio item)
		foreach (EntityAI cargoItem : allItems)
		{
			if (!cargoItem)
				continue;
			
			// Ignorar o próprio item
			if (cargoItem == item)
				continue;
			
			// Verificar se é attachment
			if (attachments.Find(cargoItem) != -1)
				continue; // É attachment, não conta como cargo
			
			// Verificar usando InventoryLocation se o item está realmente dentro deste container
			InventoryLocation itemLoc = new InventoryLocation();
			if (!cargoItem.GetInventory().GetCurrentInventoryLocation(itemLoc))
				continue; // Não conseguiu obter location, pular
			
			// Se o parent não é o item sendo verificado, não é cargo deste item
			EntityAI parent = itemLoc.GetParent();
			if (parent != item)
				continue; // Item não está dentro deste container
			
			// Item está no cargo - verificar recursivamente se ele também tem cargo
			if (HasCargoItemsRecursiveInternal(cargoItem, checkedItems))
				return true; // Container dentro de container com itens
			
			// Item no cargo encontrado
			return true;
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
	
	// Obter display name do item
	static string GetItemDisplayName(string className)
	{
		string displayName = "";
		
		GetGame().ConfigGetText("CfgVehicles " + className + " displayName", displayName);
		if (displayName && displayName != "")
			return displayName;
		
		GetGame().ConfigGetText("CfgWeapons " + className + " displayName", displayName);
		if (displayName && displayName != "")
			return displayName;
		
		GetGame().ConfigGetText("CfgMagazines " + className + " displayName", displayName);
		if (displayName && displayName != "")
			return displayName;
		
		GetGame().ConfigGetText("CfgAmmo " + className + " displayName", displayName);
		if (displayName && displayName != "")
			return displayName;
		
		return className; // Fallback
	}
	
	// Esta função não é mais usada - validação de cargo agora bloqueia venda
	// Mantida apenas para referência
}
