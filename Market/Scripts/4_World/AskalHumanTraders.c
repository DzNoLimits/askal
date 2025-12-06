// ==========================================
// ASK_TraderHumanBase - Classe base para traders humanos (NPCs)
// Inspirado no padrão do Expansion Mod
// Todas as classes individuais herdam desta base no script
// ==========================================

class ASK_TraderHumanBase extends SurvivorBase
{
	override void SetActions()
	{
		// NÃO chamar super.SetActions() para evitar ActionCheckPulseTarget
		// Adicionar apenas a ação de abrir menu do trader
		AddAction(ActionOpenAskalTraderMenu);
	}
	
	void LoadTraderConfig(string fileName)
	{
		if (!GetGame().IsServer())
			return;
		
		AskalTraderConfig config = AskalTraderConfig.Load(fileName);
		if (!config)
		{
			Print("[ASK_TraderHumanBase] ❌ Falha ao carregar config: " + fileName);
			return;
		}
		
		AskalTraderBase traderLogic = new AskalTraderBase();
		traderLogic.SetTraderEntity(this);
		traderLogic.LoadConfig(config);
		AskalTraderSpawnService.MarkAsTrader(this, traderLogic);
		AskalTraderSpawnService.AddTraderToList(traderLogic);
		Print("[ASK_TraderHumanBase] ✅ Trader configurado: " + config.TraderName);
	}
	
	AskalTraderBase GetTraderLogic()
	{
		return AskalTraderSpawnService.GetTraderFromEntity(this);
	}
}

// ==========================================
// Classes individuais de traders (apenas declarações vazias)
// No config.cpp elas herdam dos modelos visuais específicos (SurvivorM_*, SurvivorF_*)
// No script elas herdam da classe base ASK_TraderHumanBase para ter a funcionalidade
// ==========================================

class ASK_Trader_Mirek extends ASK_TraderHumanBase {};
class ASK_Trader_Boris extends ASK_TraderHumanBase {};
class ASK_Trader_Cyril extends ASK_TraderHumanBase {};
class ASK_Trader_Denis extends ASK_TraderHumanBase {};
class ASK_Trader_Elias extends ASK_TraderHumanBase {};
class ASK_Trader_Francis extends ASK_TraderHumanBase {};
class ASK_Trader_Guo extends ASK_TraderHumanBase {};
class ASK_Trader_Hassan extends ASK_TraderHumanBase {};
class ASK_Trader_Indar extends ASK_TraderHumanBase {};
class ASK_Trader_Jose extends ASK_TraderHumanBase {};
class ASK_Trader_Kaito extends ASK_TraderHumanBase {};
class ASK_Trader_Lewis extends ASK_TraderHumanBase {};
class ASK_Trader_Manua extends ASK_TraderHumanBase {};
class ASK_Trader_Niki extends ASK_TraderHumanBase {};
class ASK_Trader_Oliver extends ASK_TraderHumanBase {};
class ASK_Trader_Peter extends ASK_TraderHumanBase {};
class ASK_Trader_Quinn extends ASK_TraderHumanBase {};
class ASK_Trader_Rolf extends ASK_TraderHumanBase {};
class ASK_Trader_Seth extends ASK_TraderHumanBase {};
class ASK_Trader_Taiki extends ASK_TraderHumanBase {};
class ASK_Trader_Eva extends ASK_TraderHumanBase {};
class ASK_Trader_Frida extends ASK_TraderHumanBase {};
class ASK_Trader_Gabi extends ASK_TraderHumanBase {};
class ASK_Trader_Helga extends ASK_TraderHumanBase {};
class ASK_Trader_Irena extends ASK_TraderHumanBase {};
class ASK_Trader_Judy extends ASK_TraderHumanBase {};
class ASK_Trader_Keiko extends ASK_TraderHumanBase {};
class ASK_Trader_Linda extends ASK_TraderHumanBase {};
class ASK_Trader_Maria extends ASK_TraderHumanBase {};
class ASK_Trader_Naomi extends ASK_TraderHumanBase {};
class ASK_Trader_Baty extends ASK_TraderHumanBase {};
