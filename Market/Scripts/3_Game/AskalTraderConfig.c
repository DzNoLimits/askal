// ==========================================
// AskalTraderConfig - Configuração de Trader
// Lê e parseia arquivos Trader_Description.jsonc
// ==========================================

class AskalTraderConfig
{
	string Version; // Versão como string (ex: "1.0.0") - campo do JSON
	[NonSerialized()]
	int VersionInt; // Versão numérica (calculada a partir de Version)
	string TraderName;
	string TraderObject; // SurvivorM_*, SurvivorF_*, ou StaticObj_*
	ref array<string> TraderAttachments; // Campo correto
	ref array<string> TraderAtachments; // Campo com typo (compatibilidade)
	ref array<float> TraderPosition; // [x, y, z]
	ref array<float> TraderOrientation; // [yaw, pitch, roll]
	string AcceptedCurrency; // CurrencyID (formato string - campo do JSON)
	[NonSerialized()]
	ref map<string, int> AcceptedCurrencyMap; // CurrencyID : TransactionMode (calculado a partir de AcceptedCurrency)
	float BuyCoefficient; // Coeficiente de compra
	float SellCoefficient; // Coeficiente de venda
	ref map<string, int> SetupItems; // Dataset/Category/Item : ItemMode
	ref map<string, ref array<ref VehicleSpawnPoint>> VehicleSpawnPoints; // "Land" ou "Water" : array de pontos de spawn
	ref map<string, ref array<ref VehicleSpawnPoint>> VeHicleSpawnPoints; // Campo com typo (compatibilidade)
	
	[NonSerialized()]
	string m_FileName;
	
	void AskalTraderConfig()
	{
		Version = "1.0.0"; // Padrão
		VersionInt = 1; // Calculado a partir de Version
		TraderAttachments = new array<string>();
		TraderAtachments = new array<string>(); // Compatibilidade com typo
		TraderPosition = new array<float>();
		TraderOrientation = new array<float>();
		AcceptedCurrency = "";
		AcceptedCurrencyMap = new map<string, int>();
		BuyCoefficient = 1.0;
		SellCoefficient = 1.0;
		SetupItems = new map<string, int>();
		VehicleSpawnPoints = new map<string, ref array<ref VehicleSpawnPoint>>();
		VeHicleSpawnPoints = new map<string, ref array<ref VehicleSpawnPoint>>(); // Compatibilidade com typo
	}
	
	// Obter attachments (suporta ambos os campos)
	array<string> GetAttachments()
	{
		if (TraderAttachments && TraderAttachments.Count() > 0)
			return TraderAttachments;
		if (TraderAtachments && TraderAtachments.Count() > 0)
			return TraderAtachments; // Fallback para typo
		return new array<string>();
	}
	
	// Carregar config pelo TraderName (busca em todos os arquivos)
	static AskalTraderConfig LoadByTraderName(string traderName)
	{
		if (!traderName || traderName == "")
		{
			Print("[AskalTrader] ⚠️ TraderName inválido");
			return NULL;
		}
		
		// Obter caminho da pasta de traders
		string tradersPath = AskalTraderConfig.GetTradersPath();
		if (!FileExist(tradersPath))
		{
			Print("[AskalTrader] ⚠️ Pasta de traders não existe: " + tradersPath);
			return NULL;
		}
		
		// Listar todos os arquivos .json e .jsonc
		string fileName = "";
		FileAttr fileAttr = 0;
		string searchPattern = tradersPath + "*";
		FindFileHandle findHandle = FindFile(searchPattern, fileName, fileAttr, 0);
		
		if (!findHandle)
		{
			Print("[AskalTrader] ⚠️ Nenhum arquivo encontrado em: " + tradersPath);
			return NULL;
		}
		
		while (true)
		{
			if (fileName && fileName != "" && fileName != "." && fileName != "..")
			{
				// Verificar se é .json ou .jsonc
				if (fileName.IndexOf(".json") != -1 || fileName.IndexOf(".jsonc") != -1)
				{
					string nameWithoutExt = "";
					if (fileName.IndexOf(".jsonc") != -1)
						nameWithoutExt = fileName.Substring(0, fileName.IndexOf(".jsonc"));
					else if (fileName.IndexOf(".json") != -1)
						nameWithoutExt = fileName.Substring(0, fileName.IndexOf(".json"));
					
					if (nameWithoutExt != "")
					{
						// Tentar carregar o arquivo
						AskalTraderConfig testConfig = AskalTraderConfig.Load(nameWithoutExt);
						if (testConfig && testConfig.TraderName == traderName)
						{
							Print("[AskalTrader] ✅ Trader encontrado por TraderName: " + traderName + " (arquivo: " + nameWithoutExt + ")");
							return testConfig;
						}
					}
				}
			}
			
			// Próximo arquivo
			if (!FindNextFile(findHandle, fileName, fileAttr))
				break;
		}
		
		Print("[AskalTrader] ⚠️ Trader não encontrado por TraderName: " + traderName);
		return NULL;
	}
	
	// Carregar config de arquivo JSON
	static AskalTraderConfig Load(string fileName)
	{
		if (!fileName || fileName == "")
		{
			Print("[AskalTrader] ⚠️ Nome de arquivo inválido");
			return NULL;
		}
		
		// Remover extensão se presente
		if (fileName.IndexOf(".jsonc") != -1)
			fileName = fileName.Substring(0, fileName.IndexOf(".jsonc"));
		if (fileName.IndexOf(".json") != -1)
			fileName = fileName.Substring(0, fileName.IndexOf(".json"));
		
		// Caminho do arquivo - tentar .json primeiro, depois .jsonc
		string tradersPath = AskalTraderConfig.GetTradersPath();
		string configPath = tradersPath + fileName + ".json";
		if (!FileExist(configPath))
		{
			configPath = tradersPath + fileName + ".jsonc";
		}
		
		AskalTraderConfig config = new AskalTraderConfig();
		if (!AskalJsonLoader<AskalTraderConfig>.LoadFromFile(configPath, config, true))
		{
			Print("[AskalTrader] ❌ Falha ao carregar trader: " + fileName);
			return NULL;
		}
		
		config.m_FileName = fileName;
		
		// Normalizar Version: converter string para int
		if (config.Version != "")
		{
			// Tentar extrair número da versão (ex: "1.0.0" -> 1)
			if (config.Version.IndexOf(".") != -1)
			{
				string majorVersion = config.Version.Substring(0, config.Version.IndexOf("."));
				config.VersionInt = majorVersion.ToInt();
			}
			else
			{
				config.VersionInt = config.Version.ToInt();
			}
			// Se falhar, usar padrão
			if (config.VersionInt <= 0)
				config.VersionInt = 1;
		}
		else
		{
			// Se Version estiver vazio, usar padrão
			config.Version = "1.0.0";
			config.VersionInt = 1;
		}
		
		// Normalizar AcceptedCurrency: converter string para map
		if (config.AcceptedCurrency != "")
		{
			config.AcceptedCurrencyMap.Set(config.AcceptedCurrency, 1); // TransactionMode padrão: 1 (inventário)
		}
		else
		{
			// Se AcceptedCurrency estiver vazio, usar padrão
			config.AcceptedCurrency = "Askal_Money";
			config.AcceptedCurrencyMap.Set("Askal_Money", 1);
		}
		
		// Validar campos obrigatórios
		if (!config.Validate())
		{
			Print("[AskalTrader] ❌ Config inválido: " + fileName);
			return NULL;
		}
		
		Print("[AskalTrader] ✅ Trader carregado: " + config.TraderName + " (" + fileName + ")");
		return config;
	}
	
	// Validar campos obrigatórios
	bool Validate()
	{
		if (!TraderName || TraderName == "")
		{
			Print("[AskalTrader] ❌ TraderName não definido");
			return false;
		}
		
		if (!TraderObject || TraderObject == "")
		{
			Print("[AskalTrader] ❌ TraderObject não definido");
			return false;
		}
		
		if (!TraderPosition || TraderPosition.Count() != 3)
		{
			Print("[AskalTrader] ❌ TraderPosition inválido (deve ter 3 valores: x, y, z)");
			return false;
		}
		
		if (!TraderOrientation || TraderOrientation.Count() != 3)
		{
			Print("[AskalTrader] ❌ TraderOrientation inválido (deve ter 3 valores: yaw, pitch, roll)");
			return false;
		}
		
		return true;
	}
	
	// Obter posição como vector
	vector GetPosition()
	{
		if (!TraderPosition || TraderPosition.Count() != 3)
			return vector.Zero;
		
		return Vector(TraderPosition.Get(0), TraderPosition.Get(1), TraderPosition.Get(2));
	}
	
	// Obter orientação como vector
	vector GetOrientation()
	{
		if (!TraderOrientation || TraderOrientation.Count() != 3)
			return vector.Zero;
		
		return Vector(TraderOrientation.Get(0), TraderOrientation.Get(1), TraderOrientation.Get(2));
	}
	
	// Verificar se é objeto estático
	bool IsStatic()
	{
		if (!TraderObject || TraderObject == "")
			return false;
		
		// Aceitar qualquer objeto estático (StaticObj_, AskalTraderVendingMachine, etc.)
		if (TraderObject.IndexOf("StaticObj_") == 0)
			return true;
		
		if (TraderObject.IndexOf("AskalTrader") == 0)
			return true;
		
		return false;
	}
	
	// Obter caminho da pasta de traders
	static string GetTradersPath()
	{
		// Priorizar $mission: sobre $profile: (traders geralmente ficam na missão)
		array<string> candidatePaths = {
			"$mission:Askal/Traders/",
			"$mission:Askal\\Traders\\",
			"$profile:config/Askal/Traders/",
			"$profile:config\\Askal\\Traders\\",
			"$profile:Askal/Traders/",
			"$profile:Askal\\Traders\\"
		};
		
		foreach (string path : candidatePaths)
		{
			if (FileExist(path))
				return path;
		}
		
		// Retornar padrão da missão se nenhum existir
		return "$mission:Askal/Traders/";
	}
	
	// Obter nome do arquivo
	string GetFileName()
	{
		return m_FileName;
	}
	
	// Obter pontos de spawn de veículos (suporta ambos os campos)
	map<string, ref array<ref VehicleSpawnPoint>> GetVehicleSpawnPoints()
	{
		if (VehicleSpawnPoints && VehicleSpawnPoints.Count() > 0)
			return VehicleSpawnPoints;
		if (VeHicleSpawnPoints && VeHicleSpawnPoints.Count() > 0)
			return VeHicleSpawnPoints; // Fallback para typo
		return new map<string, ref array<ref VehicleSpawnPoint>>();
	}
	
	// Obter pontos de spawn para tipo específico (Land ou Water)
	array<ref VehicleSpawnPoint> GetVehicleSpawnPointsForType(string spawnType)
	{
		map<string, ref array<ref VehicleSpawnPoint>> spawnPoints = GetVehicleSpawnPoints();
		if (spawnPoints && spawnPoints.Contains(spawnType))
		{
			return spawnPoints.Get(spawnType);
		}
		return new array<ref VehicleSpawnPoint>();
	}
}

