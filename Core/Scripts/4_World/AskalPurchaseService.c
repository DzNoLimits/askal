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
	static bool ProcessPurchaseWithQuantity(PlayerIdentity identity, string steamId, string itemClass, int price, string currencyId, float itemQuantity, int quantityType, int contentType, string traderName = "")
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
			Print("[AskalPurchase] ❌ Balance insuficiente: " + currentBalance + " < " + price);
			return false;
		}
		
		// Verificar se é veículo (veículos devem ser spawnados no mundo, não no inventário)
		bool isVehicle = AskalVehicleSpawn.IsVehicleClass(itemClass);
		EntityAI createdItem = NULL;
		
		if (isVehicle)
		{
			// Processar spawn de veículo
			Print("[AskalPurchase] 🚗 Detectado veículo: " + itemClass + " - usando sistema de spawn");
			
			// Remover balance ANTES de spawnar (atomicidade)
			if (!AskalPlayerBalance.RemoveBalance(steamId, price, balanceKey))
			{
				Print("[AskalPurchase] ❌ Erro ao remover balance para veículo");
				return false;
			}
			
			// Tentar spawnar veículo
			bool spawnSuccess = ProcessVehiclePurchase(player, itemClass, traderName, steamId);
			
			if (!spawnSuccess)
			{
				// Rollback: reembolsar balance
				Print("[AskalPurchase] ❌ Falha ao spawnar veículo - reembolsando balance");
				AskalPlayerBalance.AddBalance(steamId, price, balanceKey);
				return false;
			}
			
			// Sucesso - veículo spawnado e balance removido
			int vehicleBalance = AskalPlayerBalance.GetBalance(steamId, balanceKey);
			Print("[AskalPurchase] ✅ Veículo spawnado com sucesso!");
			Print("[AskalPurchase]   Veículo: " + itemClass);
			Print("[AskalPurchase]   Balance atualizado: " + vehicleBalance);
			
			// Notificar cliente
			string itemDisplayName = GetItemDisplayName(itemClass);
			AskalNotificationHelper.AddPurchaseNotification(itemClass, price, itemDisplayName);
			
			return true;
		}
		else
		{
			// Item normal - criar no inventário
			createdItem = player.GetInventory().CreateInInventory(itemClass);
			if (!createdItem)
			{
				Print("[AskalPurchase] ❌ Não foi possível criar item (sem espaço no inventário): " + itemClass);
				return false;
			}
			
			// Aplicar quantidade e conteúdo ANTES dos attachments
			ApplyQuantityAndContent(createdItem, itemQuantity, quantityType, contentType);
			
			// Attachments padrão
			AttachDefaultAttachments(createdItem, itemClass);
			
			// Remover balance
			if (!AskalPlayerBalance.RemoveBalance(steamId, price, balanceKey))
			{
				Print("[AskalPurchase] ❌ Erro ao remover balance");
				GetGame().ObjectDelete(createdItem);
				return false;
			}
			
			// Sucesso - item criado e balance removido
			int newBalance = AskalPlayerBalance.GetBalance(steamId, balanceKey);
			Print("[AskalPurchase] ✅ Compra com quantidade customizada realizada!");
			Print("[AskalPurchase]   Item: " + itemClass + " | Qty: " + itemQuantity + " | Content: " + contentType);
			Print("[AskalPurchase]   Balance atualizado: " + newBalance);
			
			return true;
		}
	}
	
	// Processar compra de veículo (spawn no mundo)
	static bool ProcessVehiclePurchase(PlayerBase player, string vehicleClass, string traderName = "", string steamId = "")
	{
		if (!player || !vehicleClass || vehicleClass == "")
		{
			Print("[AskalPurchase] ❌ Parâmetros inválidos para ProcessVehiclePurchase");
			return false;
		}
		
		if (!GetGame().IsServer())
		{
			Print("[AskalPurchase] ❌ ProcessVehiclePurchase só pode ser chamado no servidor");
			return false;
		}
		
		Print("[AskalPurchase] 🚗 Processando compra de veículo: " + vehicleClass);
		
		vector spawnPos = vector.Zero;
		vector spawnRot = "0 0 0";
		
		// Verificar se trader tem pontos de spawn configurados
		AskalTraderConfig traderConfig = NULL;
		if (traderName && traderName != "")
		{
			traderConfig = AskalTraderConfig.LoadByTraderName(traderName);
		}
		
		if (traderConfig && traderConfig.VehicleSpawnPoints)
		{
			// Trader tem pontos configurados - usar primeiro ponto válido
			Print("[AskalPurchase] 🎯 Trader tem pontos de spawn configurados, tentando usar...");
			
			vector clearanceBox = AskalVehicleSpawn.GetDefaultClearanceBox();
			bool foundValidPoint = false;
			
			// Tentar pontos terrestres primeiro
			if (traderConfig.VehicleSpawnPoints.Land && traderConfig.VehicleSpawnPoints.Land.Count() > 0)
			{
				for (int landIdx = 0; landIdx < traderConfig.VehicleSpawnPoints.Land.Count(); landIdx++)
				{
					AskalVehicleSpawnPoint landSpawnPoint = traderConfig.VehicleSpawnPoints.Land.Get(landIdx);
					if (!landSpawnPoint)
						continue;
					
					vector landCandidatePos = landSpawnPoint.GetPosition();
					vector landCandidateRot = landSpawnPoint.GetRotation();
					
					if (landCandidatePos == vector.Zero)
						continue;
					
					if (AskalVehicleSpawn.IsAreaClear(landCandidatePos, clearanceBox))
					{
						spawnPos = landCandidatePos;
						spawnRot = landCandidateRot;
						if (spawnRot == vector.Zero)
							spawnRot = "0 0 0";
						foundValidPoint = true;
						Print("[AskalPurchase] ✅ Ponto de spawn terrestre válido encontrado: " + spawnPos);
						break;
					}
				}
			}
			
			// Se não encontrou ponto terrestre, tentar pontos aquáticos
			if (!foundValidPoint && traderConfig.VehicleSpawnPoints.Water && traderConfig.VehicleSpawnPoints.Water.Count() > 0)
			{
				for (int waterIdx = 0; waterIdx < traderConfig.VehicleSpawnPoints.Water.Count(); waterIdx++)
				{
					AskalVehicleSpawnPoint waterSpawnPoint = traderConfig.VehicleSpawnPoints.Water.Get(waterIdx);
					if (!waterSpawnPoint)
						continue;
					
					vector waterCandidatePos = waterSpawnPoint.GetPosition();
					vector waterCandidateRot = waterSpawnPoint.GetRotation();
					
					if (waterCandidatePos == vector.Zero)
						continue;
					
					if (AskalVehicleSpawn.IsAreaClear(waterCandidatePos, clearanceBox))
					{
						spawnPos = waterCandidatePos;
						spawnRot = waterCandidateRot;
						if (spawnRot == vector.Zero)
							spawnRot = "0 0 0";
						foundValidPoint = true;
						Print("[AskalPurchase] ✅ Ponto de spawn aquático válido encontrado: " + spawnPos);
						break;
					}
				}
			}
			
			if (!foundValidPoint)
			{
				Print("[AskalPurchase] ⚠️ Nenhum ponto de spawn válido encontrado no trader, usando fallback");
			}
		}
		
		// Se não há pontos configurados ou nenhum ponto válido, buscar posição perto do player
		if (spawnPos == vector.Zero)
		{
			Print("[AskalPurchase] 🔍 Buscando posição válida perto do player...");
			vector playerPos = player.GetPosition();
			spawnPos = AskalVehicleSpawn.FindValidSpawnPositionNearPosition(playerPos, AskalVehicleSpawn.GetDefaultRadius(), AskalVehicleSpawn.GetDefaultAttempts(), AskalVehicleSpawn.GetDefaultMaxInclination(), AskalVehicleSpawn.GetDefaultClearanceBox());
			
			// Calcular rotação baseada na direção do player
			if (spawnPos != vector.Zero)
			{
				vector direction = spawnPos - playerPos;
				direction[1] = 0; // Manter horizontal
				direction.Normalize();
				
				// Converter direção para rotação (yaw)
				// Atan2 retorna em radianos, converter para graus
				float yawRad = Math.Atan2(-direction[0], direction[2]);
				float yaw = yawRad * 57.2957795; // 180/PI aproximado
				spawnRot = Vector(yaw, 0, 0);
			}
		}
		
		// Fallback: tentar usar pontos de outros traders
		if (spawnPos == vector.Zero)
		{
			Print("[AskalPurchase] ⚠️ Nenhuma posição válida encontrada, tentando fallback...");
			// TODO: Implementar fallback para pontos de outros traders ou ponto global configurável
			Print("[AskalPurchase] ❌ Fallback não implementado - spawn de veículo falhou");
			return false;
		}
		
		// Spawnar veículo
		string ownerId = steamId;
		if (!ownerId || ownerId == "")
		{
			ownerId = player.GetIdentity().GetPlainId();
			if (!ownerId || ownerId == "")
				ownerId = player.GetIdentity().GetId();
		}
		
		bool spawnSuccess = AskalVehicleSpawn.SpawnVehicleAtPosition(vehicleClass, spawnPos, spawnRot, ownerId);
		
		if (spawnSuccess)
		{
			Print("[AskalPurchase] ✅ Veículo spawnado com sucesso em " + spawnPos);
		}
		else
		{
			Print("[AskalPurchase] ❌ Falha ao spawnar veículo em " + spawnPos);
		}
		
		return spawnSuccess;
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



