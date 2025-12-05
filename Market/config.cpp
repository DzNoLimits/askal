class CfgPatches
{
	class Askal_Market
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","DZ_Scripts","Askal_Core","JM_CF_Scripts"};
	};
};
class CfgMods
{
	class Askal_Market
	{
		dir = "Askal/Market";
		hideName = 1;
		hidePicture = 1;
		name = "Askal Market";
		credits = "Dayz Modders Community, Jacob Mango, Arkensor";
		author = "Askal";
		authorID = "0";
		version = "1.0.0";
		extra = 0;
		type = "mod";
		inputs = "Askal/Market/Scripts/Data/Inputs.xml";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"Askal/Market/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"Askal/Market/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"Askal/Market/Scripts/5_Mission"};
			};
		};
	};
};
