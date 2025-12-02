// ==========================================
// AskalTraderSpawnService - Serviço de spawn de traders
// Spawna traders dinamicamente baseado em arquivos JSON
// ==========================================

class AskalTraderSpawnService
{
	protected static ref array<ref AskalTraderBase> s_SpawnedTraders;
	
	static void Init()
	{
		if (!s_SpawnedTraders)
			s_SpawnedTraders = new array<ref AskalTraderBase>();
	}
	
	// Spawnar todos os traders
	static void SpawnAllTraders()
	{
		if (!GetGame().IsServer())
		{
			Print("[AskalTrader] ⚠️ SpawnAllTraders só pode ser chamado no servidor");
			return;
		}
		
		Init();
		
		Print("[AskalTrader] ========================================");
		Print("[AskalTrader] INICIANDO SPAWN DE TRADERS");
		Print("[AskalTrader] ========================================");
		
		// Obter caminho da pasta de traders
		string tradersPath = AskalTraderConfig.GetTradersPath();
		Print("[AskalTrader] Caminho dos traders: " + tradersPath);
		
		// Listar arquivos Trader_*.jsonc
		array<string> traderFiles = new array<string>();
		FindTraderFiles(tradersPath, traderFiles);
		
		Print("[AskalTrader] Encontrados " + traderFiles.Count() + " arquivos de trader");
		
		// Spawnar cada trader
		int tradersSpawned = 0;
		foreach (string fileName : traderFiles)
		{
			if (SpawnTrader(fileName))
				tradersSpawned++;
		}
		
		Print("[AskalTrader] ========================================");
		Print("[AskalTrader] SPAWN DE TRADERS CONCLUÍDO");
		Print("[AskalTrader] Traders iniciados: " + tradersSpawned);
		Print("[AskalTrader] (Configuração será concluída em ~50ms)");
		Print("[AskalTrader] ========================================");
	}
	
	// Encontrar arquivos de trader
	static void FindTraderFiles(string path, out array<string> files)
	{
		files.Clear();
		
		// Tentar criar pasta se não existir
		if (!FileExist(path))
		{
			Print("[AskalTrader] ⚠️ Pasta de traders não existe: " + path);
			Print("[AskalTrader] 💡 Crie a pasta manualmente e adicione arquivos Trader_*.json ou Trader_*.jsonc");
			return;
		}
		
		// Listar todos os arquivos na pasta
		string fileName = "";
		FileAttr fileAttr = 0;
		string searchPattern = path + "*";
		FindFileHandle findHandle = FindFile(searchPattern, fileName, fileAttr, 0);
		
		if (!findHandle)
		{
			Print("[AskalTrader] ⚠️ Nenhum arquivo encontrado em: " + path);
			return;
		}
		
		while (true)
		{
			// Aceitar qualquer arquivo .json ou .jsonc (não precisa começar com "Trader_")
			if (fileName && fileName != "" && fileName != "." && fileName != "..")
			{
				string nameWithoutExt = "";
				
				// Verificar se é .jsonc primeiro (mais específico), depois .json
				if (fileName.IndexOf(".jsonc") != -1)
				{
					nameWithoutExt = fileName.Substring(0, fileName.IndexOf(".jsonc"));
				}
				else if (fileName.IndexOf(".json") != -1)
				{
					nameWithoutExt = fileName.Substring(0, fileName.IndexOf(".json"));
				}
				
				if (nameWithoutExt != "")
				{
					files.Insert(nameWithoutExt);
					Print("[AskalTrader] 📄 Arquivo encontrado: " + nameWithoutExt);
				}
			}
			
			if (!FindNextFile(findHandle, fileName, fileAttr))
				break;
		}
		
		CloseFindFile(findHandle);
	}
	
	// Spawnar um trader específico (apenas objetos estáticos)
	static bool SpawnTrader(string fileName)
	{
		if (!GetGame().IsServer())
			return false;
		
		// Carregar configuração
		AskalTraderConfig config = AskalTraderConfig.Load(fileName);
		if (!config)
		{
			Print("[AskalTrader] ❌ Falha ao carregar config: " + fileName);
			return false;
		}
		
		// Criar objeto
		vector position = config.GetPosition();
		vector orientation = config.GetOrientation();
		
		// Verificar se a posição é válida
		if (position == vector.Zero)
		{
			Print("[AskalTrader] ❌ Posição inválida para trader: " + config.TraderName);
			return false;
		}
		
		// Verificar se o className é válido
		if (!config.TraderObject || config.TraderObject == "")
		{
			Print("[AskalTrader] ❌ TraderObject não definido para: " + config.TraderName);
			return false;
		}
		
		// Criar objeto estático diretamente na posição
		Object traderObj = GetGame().CreateObjectEx(config.TraderObject, position, ECE_PLACE_ON_SURFACE);
		
		if (!traderObj)
		{
			Print("[AskalTrader] ❌ Falha ao criar objeto estático: " + config.TraderObject + " em " + position);
			return false;
		}
		
		// Aplicar orientação
		traderObj.SetOrientation(orientation);
		
		// Configurar como trader após inicialização
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SetupTraderStatic, 50, false, traderObj, fileName);
		
		Print("[AskalTrader] ✅ Trader estático spawnado: " + config.TraderName + " (" + config.TraderObject + ") em " + config.GetPosition());
		
		return true;
	}
	
	// Configurar objeto estático como trader
	static void SetupTraderStatic(Object obj, string configFileName)
	{
		if (!obj)
			return;
		
		// Tentar cast para AskalTraderVendingMachine primeiro
		AskalTraderVendingMachine traderVending = AskalTraderVendingMachine.Cast(obj);
		if (traderVending)
		{
			traderVending.LoadTraderConfig(configFileName);
			return;
		}
		
		// Fallback para BuildingBase genérico
		BuildingBase traderBuilding = BuildingBase.Cast(obj);
		if (!traderBuilding)
		{
			Print("[AskalTrader] ❌ Objeto não é BuildingBase: " + obj.GetType());
			return;
		}
		
		// Carregar configuração
		AskalTraderConfig config = AskalTraderConfig.Load(configFileName);
		if (!config)
		{
			Print("[AskalTrader] ❌ Falha ao carregar config em SetupTraderStatic: " + configFileName);
			return;
		}
		
		// Criar lógica do trader
		AskalTraderBase traderLogic = new AskalTraderBase();
		traderLogic.SetTraderEntity(traderBuilding);
		traderLogic.LoadConfig(config);
		
		// Marcar como trader
		MarkAsTrader(traderBuilding, traderLogic);
		
		// Adicionar à lista
		if (!s_SpawnedTraders)
			s_SpawnedTraders = new array<ref AskalTraderBase>();
		s_SpawnedTraders.Insert(traderLogic);
		
		Print("[AskalTrader] ✅ Trader estático configurado: " + config.TraderName);
	}
	
	// Marcar entidade como trader (usando helper)
	static void MarkAsTrader(Object obj, AskalTraderBase traderLogic)
	{
		// Armazenar referência do trader logic na entidade
		// Usar EnScript para armazenar variável na entidade
		EnScript.SetClassVar(obj, "m_AskalTraderLogic", 0, traderLogic);
	}
	
	// Adicionar trader à lista (helper público)
	static void AddTraderToList(AskalTraderBase traderLogic)
	{
		if (!traderLogic)
			return;
		
		Init();
		
		// Verificar se já está na lista
		foreach (AskalTraderBase existing : s_SpawnedTraders)
		{
			if (existing == traderLogic)
				return; // Já está na lista
		}
		
		s_SpawnedTraders.Insert(traderLogic);
	}
	
	// Obter trader logic de uma entidade
	static AskalTraderBase GetTraderFromEntity(EntityAI entity)
	{
		if (!entity)
			return NULL;
		
		AskalTraderBase traderLogic;
		EnScript.GetClassVar(entity, "m_AskalTraderLogic", 0, traderLogic);
		return traderLogic;
	}
	
	// Obter todos os traders spawnados
	static array<ref AskalTraderBase> GetAllTraders()
	{
		Init();
		return s_SpawnedTraders;
	}
}

