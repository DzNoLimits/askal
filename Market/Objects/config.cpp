class CfgPatches
{
	class Askal_Market_Objects
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","DZ_Characters","DZ_Structures","DZ_Structures_Furniture","Askal_Market"};
	};
};
class CfgVehicles
{
	class HouseNoDestruct;
	class Inventory_Base;
	class SurvivorM_Mirek;
	class SurvivorM_Boris;
	class SurvivorM_Cyril;
	class SurvivorM_Denis;
	class SurvivorM_Elias;
	class SurvivorM_Francis;
	class SurvivorM_Guo;
	class SurvivorM_Hassan;
	class SurvivorM_Indar;
	class SurvivorM_Jose;
	class SurvivorM_Kaito;
	class SurvivorM_Lewis;
	class SurvivorM_Manua;
	class SurvivorM_Niki;
	class SurvivorM_Oliver;
	class SurvivorM_Peter;
	class SurvivorM_Quinn;
	class SurvivorM_Rolf;
	class SurvivorM_Seth;
	class SurvivorM_Taiki;
	class SurvivorF_Eva;
	class SurvivorF_Frida;
	class SurvivorF_Gabi;
	class SurvivorF_Helga;
	class SurvivorF_Irena;
	class SurvivorF_Judy;
	class SurvivorF_Keiko;
	class SurvivorF_Linda;
	class SurvivorF_Maria;
	class SurvivorF_Naomi;
	class SurvivorF_Baty;

	class ASK_TraderVendingMachine: HouseNoDestruct
	{
		scope = 2; 
		displayName = "Trader Vending Machine";
		descriptionShort = "NPC Trader - Vending Machine";
		model = "DZ\structures\Furniture\Eletrical_appliances\vending_machine\vending_machine.p3d";
	};
    class ASK_Coin: Inventory_Base
	{
		scope=2;
		displayName = "Créditos";
		descriptionShort = "Criado pelo governo para reestruturação do sistema monetário, o valor de cada moeda pode variar de acordo com a assinatura magnética do núcleo";
		model="\askal\market\objects\currency.p3d";
		weight= 10.0;
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=1.0;
		varQuantityMin=0.0;
		varQuantityMax=100.0;
		varQuantityDestroyOnMin=1;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100;
					healthLevels[] = 
					{
						{1.0,{"askal\market\objects\data\coin.rvmat"}},
						{0.7,{"askal\market\objects\data\coin.rvmat"}},
						{0.5,{"askal\market\objects\data\coin_damage.rvmat"}},
						{0.3,{"askal\market\objects\data\coin_damage.rvmat"}},
						{0.0,{"askal\market\objects\data\coin_destruct.rvmat"}}
					};
				};
			};
		};
		soundImpactType="metal";
	};
	//Humans Trader - Herdam dos modelos visuais específicos
	class ASK_Trader_Mirek: SurvivorM_Mirek {};
	class ASK_Trader_Boris: SurvivorM_Boris {};
	class ASK_Trader_Cyril: SurvivorM_Cyril {};
	class ASK_Trader_Denis: SurvivorM_Denis {};
	class ASK_Trader_Elias: SurvivorM_Elias {};
	class ASK_Trader_Francis: SurvivorM_Francis {};
	class ASK_Trader_Guo: SurvivorM_Guo {};
	class ASK_Trader_Hassan: SurvivorM_Hassan {};
	class ASK_Trader_Indar: SurvivorM_Indar {};
	class ASK_Trader_Jose: SurvivorM_Jose {};
	class ASK_Trader_Kaito: SurvivorM_Kaito {};
	class ASK_Trader_Lewis: SurvivorM_Lewis {};
	class ASK_Trader_Manua: SurvivorM_Manua {};
	class ASK_Trader_Niki: SurvivorM_Niki {};
	class ASK_Trader_Oliver: SurvivorM_Oliver {};
	class ASK_Trader_Peter: SurvivorM_Peter {};
	class ASK_Trader_Quinn: SurvivorM_Quinn {};
	class ASK_Trader_Rolf: SurvivorM_Rolf {};
	class ASK_Trader_Seth: SurvivorM_Seth {};
	class ASK_Trader_Taiki: SurvivorM_Taiki {};
	class ASK_Trader_Eva: SurvivorF_Eva {};
	class ASK_Trader_Frida: SurvivorF_Frida {};
	class ASK_Trader_Gabi: SurvivorF_Gabi {};
	class ASK_Trader_Helga: SurvivorF_Helga {};
	class ASK_Trader_Irena: SurvivorF_Irena {};
	class ASK_Trader_Judy: SurvivorF_Judy {};
	class ASK_Trader_Keiko: SurvivorF_Keiko {};
	class ASK_Trader_Linda: SurvivorF_Linda {};
	class ASK_Trader_Maria: SurvivorF_Maria {};
	class ASK_Trader_Naomi: SurvivorF_Naomi {};
	class ASK_Trader_Baty: SurvivorF_Baty {};
};