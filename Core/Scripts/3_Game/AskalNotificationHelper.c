// ==========================================
// AskalNotificationHelper - Helper para notificações (3_Game)
// Permite que módulos 3_Game notifiquem o menu 5_Mission
// ==========================================

class AskalNotificationData
{
	string ActionType; // "COMPRA" ou "VENDA"
	string ItemClassName;
	string Description; // Descrição detalhada (inclui attachments, etc)
	int Price;
	bool IsPurchase;
	
	void AskalNotificationData(string actionType, string itemClass, string description, int price, bool isPurchase)
	{
		ActionType = actionType;
		ItemClassName = itemClass;
		Description = description;
		Price = price;
		IsPurchase = isPurchase;
	}
}

class AskalNotificationHelper
{
	protected static ref array<ref AskalNotificationData> s_PendingNotifications;
	
	static void Init()
	{
		if (!s_PendingNotifications)
			s_PendingNotifications = new array<ref AskalNotificationData>();
	}
	
	static void AddPurchaseNotification(string itemClassName, int price, string description = "")
	{
		Init();
		if (!description || description == "")
			description = itemClassName;
		s_PendingNotifications.Insert(new AskalNotificationData("COMPRA", itemClassName, description, price, true));
		Print("[AskalNotification] 📢 Notificação de compra adicionada: " + itemClassName + " ($" + price.ToString() + ")");
	}
	
	static void AddSellNotification(string itemClassName, int price, string description = "")
	{
		Init();
		if (!description || description == "")
			description = itemClassName;
		s_PendingNotifications.Insert(new AskalNotificationData("VENDA", itemClassName, description, price, false));
		Print("[AskalNotification] 📢 Notificação de venda adicionada: " + description + " ($" + price.ToString() + ")");
	}
	
	static array<ref AskalNotificationData> GetPendingNotifications()
	{
		Init();
		return s_PendingNotifications;
	}
	
	static void ClearPendingNotifications()
	{
		if (s_PendingNotifications)
			s_PendingNotifications.Clear();
	}
	
	static void RemoveNotification(int index)
	{
		if (s_PendingNotifications && index >= 0 && index < s_PendingNotifications.Count())
		{
			s_PendingNotifications.Remove(index);
		}
	}
	
	// ========================================
	// SISTEMA DE HEALTH DOS ITENS
	// ========================================
	
	protected static ref array<ref Param2<string, float>> s_InventoryHealth;
	
	static void SetInventoryHealth(ref array<ref Param2<string, float>> healthArray)
	{
		if (!s_InventoryHealth)
			s_InventoryHealth = new array<ref Param2<string, float>>();
		
		s_InventoryHealth.Clear();
		
		if (healthArray)
		{
			for (int i = 0; i < healthArray.Count(); i++)
			{
				Param2<string, float> healthData = healthArray.Get(i);
				if (healthData)
					s_InventoryHealth.Insert(healthData);
			}
		}
		
		Print("[AskalNotification] 💚 Health armazenado para " + s_InventoryHealth.Count() + " itens");
	}
	
	static array<ref Param2<string, float>> GetInventoryHealth()
	{
		if (!s_InventoryHealth)
			s_InventoryHealth = new array<ref Param2<string, float>>();
		
		return s_InventoryHealth;
	}
	
	static void ClearInventoryHealth()
	{
		if (s_InventoryHealth)
			s_InventoryHealth.Clear();
	}
	
	// ========================================
	// SISTEMA DE ABERTURA DE MENU DO TRADER
	// ========================================
	
	protected static string s_PendingTraderMenu;
	protected static ref map<string, int> s_PendingTraderSetupItems;
	
	static void RequestOpenTraderMenu(string traderName, ref map<string, int> setupItems = NULL)
	{
		s_PendingTraderMenu = traderName;
		if (setupItems)
		{
			s_PendingTraderSetupItems = setupItems;
		}
		else
		{
			s_PendingTraderSetupItems = new map<string, int>();
		}
		Print("[AskalNotification] 🏪 Solicitação de abertura de menu do trader: " + traderName);
		
		// Contar entradas do SetupItems (evitar operador ternário)
		int setupCount = 0;
		if (s_PendingTraderSetupItems)
			setupCount = s_PendingTraderSetupItems.Count();
		Print("[AskalNotification] 📦 SetupItems: " + setupCount.ToString() + " entradas");
	}
	
	static string GetPendingTraderMenu()
	{
		return s_PendingTraderMenu;
	}
	
	static map<string, int> GetPendingTraderSetupItems()
	{
		if (!s_PendingTraderSetupItems)
			s_PendingTraderSetupItems = new map<string, int>();
		return s_PendingTraderSetupItems;
	}
	
	static void ClearPendingTraderMenu()
	{
		s_PendingTraderMenu = "";
		if (s_PendingTraderSetupItems)
			s_PendingTraderSetupItems.Clear();
	}
}

