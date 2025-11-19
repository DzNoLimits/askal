class CfgPatches
{
	class Askal_Traders
	{
		units[] = {"AskalTraderVendingMachine"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Structures_Furniture"};
	};
};

class CfgVehicles
{
	class HouseNoDestruct;
	
	// Trader estático baseado no vending machine
	class AskalTraderVendingMachine: HouseNoDestruct
	{
		scope = 2; // Public, pode ser usado
		displayName = "Trader Vending Machine";
		descriptionShort = "NPC Trader - Vending Machine";
		model = "DZ\structures\Furniture\Eletrical_appliances\vending_machine\vending_machine.p3d";
	};
};
