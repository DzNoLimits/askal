// ==========================================
// AskalCurrencyInventoryManager - Sistema de moedas físicas no inventário
// IMPORTANTE: Este arquivo PRECISA estar em 4_World para acessar PlayerBase
// ==========================================

class AskalCurrencyInventoryManager
{
	// Conta todas as moedas de uma currency específica no inventário do player
	static int CountPhysicalCurrency(PlayerBase player, string currencyId)
	{
		if (!player)
			return 0;
		
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		if (!marketConfig)
			return 0;
		
		AskalCurrencyConfig currencyConfig = marketConfig.GetCurrencyConfig(currencyId);
		if (!currencyConfig || !currencyConfig.Values)
			return 0;
		
		int totalAmount = 0;
		
		// Escaneia todo o inventário recursivamente
		array<EntityAI> itemsInInventory = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsInInventory);
		
		foreach (EntityAI inventoryItem : itemsInInventory)
		{
			if (!inventoryItem)
				continue;
			
			string itemClass = inventoryItem.GetType();
			
			// Verifica se é uma moeda desta currency
			foreach (AskalCurrencyValueConfig valueConfig : currencyConfig.Values)
			{
				if (valueConfig.Name == itemClass)
				{
					totalAmount += valueConfig.Value;
					break;
				}
			}
		}
		
		Print("[AskalCurrency] 💰 Player tem " + totalAmount + " " + currencyId + " físico no inventário");
		return totalAmount;
	}
	
	// Remove moedas físicas do inventário (para pagamento)
	static bool RemovePhysicalCurrency(PlayerBase player, int amountToRemove, string currencyId)
	{
		if (!player || amountToRemove <= 0)
			return false;
		
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		if (!marketConfig)
			return false;
		
		AskalCurrencyConfig currencyConfig = marketConfig.GetCurrencyConfig(currencyId);
		if (!currencyConfig || !currencyConfig.Values)
			return false;
		
		// Verifica se player tem saldo suficiente
		int currentBalance = CountPhysicalCurrency(player, currencyId);
		if (currentBalance < amountToRemove)
		{
			Print("[AskalCurrency] ❌ Saldo insuficiente: " + currentBalance + " < " + amountToRemove);
			return false;
		}
		
		// Ordena moedas por valor (menor para maior)
		array<ref AskalCurrencyValueConfig> sortedValues = new array<ref AskalCurrencyValueConfig>();
		foreach (AskalCurrencyValueConfig val : currencyConfig.Values)
		{
			sortedValues.Insert(val);
		}
		SortCurrencyValues(sortedValues, false); // Menor primeiro
		
		// Remove moedas (greedy: menor denominação primeiro para minimizar troco)
		int remainingToRemove = amountToRemove;
		array<EntityAI> itemsToDelete = new array<EntityAI>();
		
		foreach (AskalCurrencyValueConfig valueConfig : sortedValues)
		{
			if (remainingToRemove <= 0)
				break;
			
			// Encontra todas as moedas desta denominação
			array<EntityAI> coinsOfThisDenom = FindCoinsInInventory(player, valueConfig.Name);
			
			foreach (EntityAI coin : coinsOfThisDenom)
			{
				if (remainingToRemove <= 0)
					break;
				
				itemsToDelete.Insert(coin);
				remainingToRemove -= valueConfig.Value;
			}
		}
		
		// Deleta as moedas
		foreach (EntityAI itemToDelete : itemsToDelete)
		{
			GetGame().ObjectDelete(itemToDelete);
		}
		
		// Se sobrou valor negativo, precisa dar troco
		if (remainingToRemove < 0)
		{
			int changeAmount = Math.AbsInt(remainingToRemove);
			Print("[AskalCurrency] 💵 Dando troco de " + changeAmount);
			AddPhysicalCurrency(player, changeAmount, currencyId);
		}
		
		Print("[AskalCurrency] ✅ Removido " + amountToRemove + " " + currencyId + " do inventário");
		return true;
	}
	
	// Adiciona moedas físicas ao inventário (com troco otimizado)
	static bool AddPhysicalCurrency(PlayerBase player, int amountToAdd, string currencyId)
	{
		if (!player || amountToAdd <= 0)
			return false;
		
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		if (!marketConfig)
			return false;
		
		AskalCurrencyConfig currencyConfig = marketConfig.GetCurrencyConfig(currencyId);
		if (!currencyConfig || !currencyConfig.Values)
			return false;
		
		// Calcula troco otimizado (maior denominação primeiro)
		array<ref Param2<string, int>> change = CalculateChange(amountToAdd, currencyConfig.Values);
		
		// Verificar se CalculateChange retornou valores válidos
		if (!change || change.Count() == 0)
		{
			Print("[AskalCurrency] ❌ ERRO: CalculateChange retornou vazio para " + amountToAdd + " " + currencyId);
			Print("[AskalCurrency] ❌ Verifique se a moeda está configurada corretamente no MarketConfig");
			return false;
		}
		
		Print("[AskalCurrency] 💰 Calculando troco para " + amountToAdd + " " + currencyId + " - " + change.Count() + " tipos de moedas");
		
		// Tenta adicionar ao inventário
		int coinsSpawned = 0;
		array<EntityAI> droppedCoins = new array<EntityAI>();
		int totalCoinsToSpawn = 0;
		
		foreach (Param2<string, int> coinEntry : change)
		{
			string coinClass = coinEntry.param1;
			int coinCount = coinEntry.param2;
			totalCoinsToSpawn += coinCount;
			
			Print("[AskalCurrency] 💰 Spawnando " + coinCount + "x " + coinClass);
			
			for (int i = 0; i < coinCount; i++)
			{
				EntityAI coin = player.GetInventory().CreateInInventory(coinClass);
				if (!coin)
				{
					// Não há espaço: dropa no chão (Q6 - OPÇÃO C)
					vector playerPos = player.GetPosition();
					playerPos[1] = playerPos[1] + 0.5; // Elevar um pouco
					Object coinObj = GetGame().CreateObjectEx(coinClass, playerPos, ECE_PLACE_ON_SURFACE);
					coin = EntityAI.Cast(coinObj);
					if (coin)
					{
						droppedCoins.Insert(coin);
						Print("[AskalCurrency] ⚠️ Moeda dropada no chão: " + coinClass);
					}
					else
					{
						Print("[AskalCurrency] ❌ ERRO CRÍTICO: Falha ao criar moeda no chão: " + coinClass);
					}
				}
				else
				{
					coinsSpawned++;
				}
			}
		}
		
		if (droppedCoins.Count() > 0)
		{
			Print("[AskalCurrency] ⚠️ " + droppedCoins.Count() + " moedas dropadas no chão (inventário cheio)");
		}
		
		// Verificar se pelo menos algumas moedas foram spawnadas
		if (coinsSpawned == 0 && droppedCoins.Count() == 0)
		{
			Print("[AskalCurrency] ❌ ERRO: Nenhuma moeda foi spawnada! Total esperado: " + totalCoinsToSpawn);
			return false;
		}
		
		Print("[AskalCurrency] ✅ Adicionado " + amountToAdd + " " + currencyId + " (" + coinsSpawned + " no inventário, " + droppedCoins.Count() + " no chão)");
		return true;
	}
	
	// Calcula troco (algoritmo greedy - maior denominação primeiro)
	static array<ref Param2<string, int>> CalculateChange(int amount, array<ref AskalCurrencyValueConfig> denominations)
	{
		array<ref Param2<string, int>> result = new array<ref Param2<string, int>>();
		
		// Ordena denominações (maior valor primeiro)
		array<ref AskalCurrencyValueConfig> sorted = new array<ref AskalCurrencyValueConfig>();
		foreach (AskalCurrencyValueConfig denominationItem : denominations)
		{
			sorted.Insert(denominationItem);
		}
		SortCurrencyValues(sorted, true); // Maior primeiro
		
		int remaining = amount;
		
		foreach (AskalCurrencyValueConfig sortedDenom : sorted)
		{
			if (remaining <= 0)
				break;
			
			int count = Math.Floor(remaining / sortedDenom.Value);
			if (count > 0)
			{
				result.Insert(new Param2<string, int>(sortedDenom.Name, count));
				remaining -= count * sortedDenom.Value;
			}
		}
		
		return result;
	}
	
	// Ordena array de valores de moeda (bubble sort simples)
	static void SortCurrencyValues(array<ref AskalCurrencyValueConfig> values, bool descending)
	{
		int n = values.Count();
		for (int i = 0; i < n - 1; i++)
		{
			for (int j = 0; j < n - i - 1; j++)
			{
				bool shouldSwap = false;
				if (descending)
					shouldSwap = values[j].Value < values[j + 1].Value;
				else
					shouldSwap = values[j].Value > values[j + 1].Value;
				
				if (shouldSwap)
				{
					AskalCurrencyValueConfig temp = values[j];
					values.Set(j, values[j + 1]);
					values.Set(j + 1, temp);
				}
			}
		}
	}
	
	// Encontra todas as moedas de um tipo específico no inventário
	static array<EntityAI> FindCoinsInInventory(PlayerBase player, string coinClass)
	{
		array<EntityAI> result = new array<EntityAI>();
		
		array<EntityAI> itemsInInventory = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsInInventory);
		
		foreach (EntityAI item : itemsInInventory)
		{
			if (item && item.GetType() == coinClass)
			{
				result.Insert(item);
			}
		}
		
		return result;
	}
}

