	// ==========================================
// AskalPurchaseService - Serviço de compra (4_World - acesso a PlayerBase)
// ==========================================

class AskalPurchaseService
{
	// Processar compra COM quantidade e conteúdo customizados
	static bool ProcessPurchaseWithQuantity(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId, float itemQuantity, int quantityType, int contentType)
	{
		Print("[AskalPurchase] 🔍 ProcessPurchaseWithQuantity: INICIANDO COMPRA");
		Print("[AskalPurchase] 🔍 Parâmetros: steamId=" + steamId + " itemClass=" + itemClass + " price=" + price + " currencyId=" + currencyId);
		
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
		
		Print("[AskalPurchase] 🔍 ProcessPurchaseWithQuantity: steamId resolvido: " + steamId);

		// Obter player (only needed for Mode 1 - physical currency)
		PlayerBase player = GetPlayerFromIdentity(identity);
		if (!player)
		{
			Print("[AskalPurchase] ❌ Player não encontrado");
			return false;
		}

		// Resolve currency ID (should already be resolved by caller, but validate)
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		if (!currencyId || currencyId == "")
		{
			if (marketConfig)
				currencyId = marketConfig.GetDefaultCurrencyId();
			else
				currencyId = "Askal_Money";
		}

		Print("[AskalPurchase] 🔍 Validating currency: " + currencyId);
		
		// Get currency config and validate it exists
		AskalCurrencyConfig currencyConfig = NULL;
		if (marketConfig)
		{
			Print("[AskalPurchase] 🔍 MarketConfig found, checking for currency: " + currencyId);
			Print("[AskalPurchase] 🔍 Available currencies count: " + marketConfig.Currencies.Count());
			currencyConfig = marketConfig.GetCurrencyConfig(currencyId);
			
			// If currency not found, try default currency as fallback
			if (!currencyConfig)
			{
				string defaultCurrencyId = marketConfig.GetDefaultCurrencyId();
				Print("[AskalPurchase] ⚠️ Currency '" + currencyId + "' not found, default currency is: " + defaultCurrencyId);
				if (defaultCurrencyId && defaultCurrencyId != "" && defaultCurrencyId != currencyId)
				{
					Print("[AskalPurchase] ⚠️ Falling back to default currency: " + defaultCurrencyId);
					currencyId = defaultCurrencyId;
					currencyConfig = marketConfig.GetCurrencyConfig(currencyId);
					if (currencyConfig)
						Print("[AskalPurchase] ✅ Fallback currency found and validated");
				}
			}
			else
			{
				Print("[AskalPurchase] ✅ Currency config found for: " + currencyId);
			}
		}
		else
		{
			Print("[AskalPurchase] ⚠️ MarketConfig not available");
		}
		
		if (!currencyConfig)
		{
			Print("[AskalPurchase] ERROR: Currency not found: " + currencyId + " (and no default currency available)");
			return false;
		}

		// Check currency Mode
		int currencyMode = currencyConfig.Mode;
		if (currencyMode == AskalMarketConstants.CURRENCY_MODE_DISABLED)
		{
			Print("[AskalPurchase] ERROR: Currency disabled: " + currencyId);
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
			Print("[AskalPurchase] ❌ Falha ao reservar funds: " + steamId + " | Amount: " + price + " | Currency: " + currencyId);
			return false;
		}

		// Handle based on currency Mode
		if (currencyMode == AskalMarketConstants.CURRENCY_MODE_VIRTUAL)
		{
			// Mode 2 (Virtual): No item spawn, just numeric balance change
			Print("[AskalPurchase] Currency mode=2 (VIRTUAL) -> no item spawn requested");
			
			// Confirm reservation (deduct balance)
			if (!AskalPlayerBalance.ConfirmReservation(steamId, price, currencyId))
			{
				Print("[AskalPurchase] ❌ Erro ao confirmar reserva para currency virtual");
				AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
				return false;
			}

			Print("[AskalPurchase] ORDER_PLACED steamId=" + steamId + " item=" + itemClass + " price=" + price + " currency=" + currencyId);
			Print("[AskalPurchase] ✅ Compra realizada (virtual currency)!");
			Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Tipo: " + quantityType);
			return true;
		}
		else if (currencyMode == AskalMarketConstants.CURRENCY_MODE_PHYSICAL)
		{
			// Mode 1 (Physical): Spawn items via Values mapping
			Print("[AskalPurchase] Currency mode=1 (PHYSICAL) -> attempting spawn via Values mapping");
			
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

			Print("[AskalPurchase] ORDER_PLACED steamId=" + steamId + " item=" + itemClass + " price=" + price + " currency=" + currencyId);
			Print("[AskalPurchase] ✅ Compra realizada!");
			Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Tipo: " + quantityType);
			return true;
		}
		else
		{
			Print("[AskalPurchase] ❌ Currency mode inválido: " + currencyMode + " para currency: " + currencyId);
			AskalPlayerBalance.ReleaseReservation(steamId, price, currencyId);
			return false;
		}
	}

	// Criar item simples (limpo e direto)
	static EntityAI CreateSimpleItem(PlayerBase player, string itemClass, float quantity, int quantityType, int contentType)
	{
		Print("[AskalPurchase] 🛠️ Criando item: " + itemClass);

		// Criar item no inventário
		EntityAI item = player.GetInventory().CreateInInventory(itemClass);
		if (!item)
		{
			// Tentar criar nas mãos se não couber no inventário
			item = player.GetHumanInventory().CreateInHands(itemClass);
		}

		if (!item)
		{
			// Última tentativa: criar no chão próximo ao player
			vector playerPos = player.GetPosition();
			Object itemObj = GetGame().CreateObjectEx(itemClass, playerPos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS);
			item = EntityAI.Cast(itemObj);
			
			if (!item)
			{
				Print("[AskalPurchase] ❌ Falha ao criar item: " + itemClass + " (inventário cheio e falha ao criar no chão)");
				return NULL;
			}
			
			Print("[AskalPurchase] ⚠️ Item criado no chão (inventário cheio): " + itemClass);
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
					Print("[AskalPurchase] 🔫 Magazine: " + ammoCount + " balas");
				}
			}
			else if (quantityType == 2 && itemBase.HasQuantity()) // STACKABLE
			{
				// Para stackables, definir quantidade diretamente
				float qtyMin = itemBase.GetQuantityMin();
				float qtyMax = itemBase.GetQuantityMax();
				float clampedQty = Math.Clamp(quantity, qtyMin, qtyMax);
				itemBase.SetQuantity(clampedQty);
				Print("[AskalPurchase] 📦 STACKABLE: " + clampedQty + " unidades (min: " + qtyMin + ", max: " + qtyMax + ")");
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
					float clampedQtyPercent = Math.Clamp(quantity, itemBase.GetQuantityMin(), itemBase.GetQuantityMax());
					itemBase.SetQuantity(clampedQtyPercent);
					Print("[AskalPurchase] 📊 Quantidade: " + clampedQtyPercent);
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
				Print("[AskalPurchase] ⚠️ Falha ao anexar attachment: " + attachmentClass);
			}
		}
	}
}

