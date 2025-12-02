// ==========================================
// AskalPurchaseService - ServiÃ§o de compra (4_World - acesso a PlayerBase)
// ==========================================

class AskalPurchaseService
{
	// Processar compra de item (chamado via RPC do servidor)
	// Retorna true se sucesso, false se erro (mensagem serÃ¡ logada)
	static bool ProcessPurchase(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId)
	{
		if (!identity)
		{
			Print("[AskalPurchase] âŒ Player identity nÃ£o encontrada");
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
			Print("[AskalPurchase] âŒ Player nÃ£o encontrado");
			return false;
		}
		
		// Resolve balance key from currencyId
		string balanceKey = AskalPlayerBalance.ResolveBalanceKey(currencyId);
		if (!balanceKey || balanceKey == "")
		{
			Print("[AskalPurchase] ❌ Failed to resolve balance key for currency: " + currencyId);
			return false;
		}
		
		// Validate currency exists in player balance
		AskalPlayerData playerData = AskalPlayerBalance.LoadPlayerData(steamId);
		if (!playerData || !playerData.Balance || !playerData.Balance.Contains(balanceKey))
		{
			Print("[AskalPurchase] ❌ Player balance missing currency key: " + balanceKey + " (currencyId: " + currencyId + ")");
			return false;
		}

		int authoritativePrice = ComputeItemTotalPrice(itemClass);
		Print("[AskalPurchase] [DEBUG] Preço calculado para " + itemClass + ": " + authoritativePrice);
		if (authoritativePrice <= 0)
		{
			Print("[AskalPurchase] âŒ PreÃ§o autoritativo invÃ¡lido para " + itemClass + ": " + authoritativePrice);
			return false;
		}
		if (price != authoritativePrice)
		{
			Print("[AskalPurchase] âš ï¸ Ajustando preÃ§o informado pelo cliente. Recebido: " + price + " | Autoritativo: " + authoritativePrice);
			price = authoritativePrice;
		}

		// Verificar balance
		int currentBalance = playerData.Balance.Get(balanceKey);
		Print("[AskalPurchase] [DEBUG] Balance atual: " + currentBalance + " (" + balanceKey + ") | Preço necessário: " + price);
		if (currentBalance < price)
		{
			Print("[AskalPurchase] âŒ Balance insuficiente: " + currentBalance + " < " + price);
			return false;
		}
		
		// Tentar criar item no inventÃ¡rio (valida espaÃ§o automaticamente)
		EntityAI createdItem = player.GetInventory().CreateInInventory(itemClass);
		if (!createdItem)
		{
			Print("[AskalPurchase] âŒ NÃ£o foi possÃ­vel criar item (sem espaÃ§o no inventÃ¡rio): " + itemClass);
			return false;
		}

		AttachDefaultAttachments(createdItem, itemClass);
		
		// Remover balance
		if (!AskalPlayerBalance.RemoveBalance(steamId, price, balanceKey))
		{
			Print("[AskalPurchase] âŒ Erro ao remover balance");
			// Rollback: deletar item criado
			GetGame().ObjectDelete(createdItem);
			return false;
		}
		
		int newBalance = AskalPlayerBalance.GetBalance(steamId, balanceKey);
	Print("[AskalPurchase] âœ… Compra realizada com sucesso!");
	Print("[AskalPurchase]   Item criado: " + itemClass);
	Print("[AskalPurchase]   Balance atualizado: " + newBalance);
	
	// Obter display name do item para notificação
	string itemDisplayName = GetItemDisplayName(itemClass);
	
	// Notificar cliente (apenas no menu)
	AskalNotificationHelper.AddPurchaseNotification(itemClass, price, itemDisplayName);
	
	return true;
}

	// Processar compra COM quantidade e conteÃºdo customizados
	static bool ProcessPurchaseWithQuantity(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId, float itemQuantity, int quantityType, int contentType)
	{
		if (!identity)
		{
			Print("[AskalPurchase] âŒ Player identity nÃ£o encontrada");
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
			Print("[AskalPurchase] âŒ Player nÃ£o encontrado");
			return false;
		}
		
		// Resolve balance key from currencyId
		string balanceKey = AskalPlayerBalance.ResolveBalanceKey(currencyId);
		if (!balanceKey || balanceKey == "")
		{
			Print("[AskalPurchase] ❌ Failed to resolve balance key for currency: " + currencyId);
			return false;
		}
		
		// Validate currency exists in player balance
		AskalPlayerData playerData = AskalPlayerBalance.LoadPlayerData(steamId);
		if (!playerData || !playerData.Balance || !playerData.Balance.Contains(balanceKey))
		{
			Print("[AskalPurchase] ❌ Player balance missing currency key: " + balanceKey + " (currencyId: " + currencyId + ")");
			return false;
		}

		int authoritativePrice = ComputeItemTotalPrice(itemClass);
		Print("[AskalPurchase] [DEBUG] Preço calculado para " + itemClass + ": " + authoritativePrice);
		if (authoritativePrice <= 0)
		{
			Print("[AskalPurchase] âŒ PreÃ§o autoritativo invÃ¡lido para " + itemClass + ": " + authoritativePrice);
			return false;
		}
		if (price != authoritativePrice)
		{
			Print("[AskalPurchase] âš ï¸ Ajustando preÃ§o informado pelo cliente. Recebido: " + price + " | Autoritativo: " + authoritativePrice);
			price = authoritativePrice;
		}

		// Verificar balance
		int currentBalance = playerData.Balance.Get(balanceKey);
		Print("[AskalPurchase] [DEBUG] Balance atual: " + currentBalance + " (" + balanceKey + ") | Preço necessário: " + price);
		if (currentBalance < price)
		{
			Print("[AskalPurchase] âŒ Balance insuficiente: " + currentBalance + " < " + price);
			return false;
		}
		
		// Tentar criar item no inventÃ¡rio
		EntityAI createdItem = player.GetInventory().CreateInInventory(itemClass);
		if (!createdItem)
		{
			Print("[AskalPurchase] âŒ NÃ£o foi possÃ­vel criar item (sem espaÃ§o no inventÃ¡rio): " + itemClass);
			return false;
		}

		// Aplicar quantidade e conteÃºdo ANTES dos attachments
		ApplyQuantityAndContent(createdItem, itemQuantity, quantityType, contentType);
		
		// Attachments padrÃ£o
		AttachDefaultAttachments(createdItem, itemClass);
		
		// Remover balance
		if (!AskalPlayerBalance.RemoveBalance(steamId, price, balanceKey))
		{
			Print("[AskalPurchase] âŒ Erro ao remover balance");
			GetGame().ObjectDelete(createdItem);
			return false;
		}
		
		int newBalance = AskalPlayerBalance.GetBalance(steamId, balanceKey);
		Print("[AskalPurchase] âœ… Compra com quantidade customizada realizada!");
		Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Content: " + contentType);
		Print("[AskalPurchase]   Balance atualizado: " + newBalance);
		
		return true;
	}
	
	// Aplicar quantidade e conteÃºdo ao item (inspirado no COT)
	static void ApplyQuantityAndContent(EntityAI entity, float quantity, int quantityType, int contentType)
	{
		if (!entity || quantity < 0)
			return;
		
		ItemBase item = ItemBase.Cast(entity);
		if (!item)
			return;
		
		// quantityType: 0=NONE, 1=MAGAZINE, 2=STACKABLE, 3=QUANTIFIABLE
		switch (quantityType)
		{
			case 1: // MAGAZINE
			{
				Magazine mag = Magazine.Cast(item);
				if (mag)
				{
					int ammoCount = Math.Round(quantity);
					mag.ServerSetAmmoCount(ammoCount);
					Print("[AskalPurchase] ðŸ”« Magazine preenchido com " + ammoCount + " balas");
				}
				break;
			}
			
			case 2: // STACKABLE
			{
				if (item.HasQuantity())
				{
					item.SetQuantity(quantity);
					Print("[AskalPurchase] ðŸ“¦ Stackable quantity setada: " + quantity);
				}
				break;
			}
			
			case 3: // QUANTIFIABLE (Bottle_Base com lÃ­quido)
			{
				if (item.IsLiquidContainer() && contentType > 0)
				{
					// Define o tipo de lÃ­quido
					item.SetLiquidType(contentType);
					
					// Calcula quantidade em mL baseado no percentual
					float maxCapacity = item.GetQuantityMax();
					float percentDecimal = quantity / 100.0;
					float liquidAmount = maxCapacity * percentDecimal;
					item.SetQuantity(liquidAmount);
					
					Print("[AskalPurchase] ðŸ’§ LÃ­quido aplicado - Type: " + contentType + " | Amount: " + liquidAmount + " mL (" + quantity + "%)");
				}
				else if (item.HasQuantity())
				{
					// Outros quantifiables sem lÃ­quido
					item.SetQuantity(quantity);
					Print("[AskalPurchase] ðŸ“Š Quantity setada: " + quantity);
				}
				break;
			}
		}
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

	protected static int ComputeItemTotalPrice(string itemClass)
	{
		ItemData itemData = AskalDatabase.GetItem(itemClass);
		if (!itemData)
			return -1;

		int totalPrice = NormalizeBuyPrice(itemData.Price);
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
					totalPrice += NormalizeBuyPrice(attachmentData.Price);
			}
		}

		return ApplyBuyCoefficient(totalPrice);
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

    protected static void AttachDefaultAttachments(EntityAI itemEntity, string itemClass)
    {
        if (!itemEntity)
            return;

        ItemData itemData = AskalDatabase.GetItem(itemClass);
        if (!itemData)
            return;

        array<string> attachments = itemData.GetAttachments();
        if (!attachments)
            return;

        for (int i = 0; i < attachments.Count(); i++)
        {
            string attachmentClass = attachments.Get(i);
            if (!attachmentClass || attachmentClass == "")
                continue;

            EntityAI attachmentEntity = EntityAI.Cast(itemEntity.GetInventory().CreateAttachment(attachmentClass));
            if (!attachmentEntity)
            {
                Print("[AskalPurchase] âš ï¸ Falha ao anexar attachment padrÃ£o: " + attachmentClass);
            }
        }
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
}



