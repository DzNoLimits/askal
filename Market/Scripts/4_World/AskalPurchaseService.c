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

		// Verificar balance
		int currentBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);
		if (currentBalance < price)
		{
			Print("[AskalPurchase] ❌ Balance insuficiente: " + currentBalance + " < " + price);
			return false;
		}

		// Criar item
		EntityAI createdItem = CreateSimpleItem(player, itemClass, itemQuantity, quantityType, contentType);
		if (!createdItem)
		{
			Print("[AskalPurchase] ❌ Não foi possível criar item: " + itemClass);
			return false;
		}

		// Attachments padrão (apenas se item foi criado com sucesso)
		AttachDefaultAttachments(createdItem, itemClass);

		// Remover balance
		if (!AskalPlayerBalance.RemoveBalance(steamId, price, currencyId))
		{
			Print("[AskalPurchase] ❌ Erro ao remover balance");
			GetGame().ObjectDelete(createdItem);
			return false;
		}

		int newBalance = AskalPlayerBalance.GetBalance(steamId, currencyId);
		Print("[AskalPurchase] ✅ Compra realizada!");
		Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Tipo: " + quantityType);
		Print("[AskalPurchase]   Balance atualizado: " + newBalance);

		return true;
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

