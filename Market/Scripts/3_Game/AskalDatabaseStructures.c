// ==========================================
// ESTRUTURAS DE DADOS PARA O DATABASE ASKAL
// ==========================================

/// Enum para tipos de quantidade de itens
enum AskalItemQuantityType
{
	NONE = 0,        // Item normal (sem quantidade variável)
	MAGAZINE = 1,    // Carregador de arma (munição)
	STACKABLE = 2,   // Itens empilháveis (pregos, balas, etc)
	QUANTIFIABLE = 3 // Itens com quantidade fracionária (bandagens, meat, water, etc)
}

// Representa um item individual no database
class AskalItemData
{
	protected string m_ClassName;
	protected string m_DisplayName;
	protected string m_Category;
	protected int m_Price;
	protected int m_BasePrice;
	protected int m_Cost;
	protected int m_Restock;
	protected int m_Lifetime;
	protected ref map<string, bool> m_Flags;
	protected ref array<string> m_Variants;
	protected ref map<string, string> m_Attachments;
	protected ref array<string> m_Keywords;
	protected ref array<string> m_DefaultAttachments;
	
	void AskalItemData()
	{
		m_Flags = new map<string, bool>;
		m_Variants = new array<string>;
		m_Attachments = new map<string, string>;
		m_Keywords = new array<string>;
		m_DefaultAttachments = new array<string>;
		m_BasePrice = 0;
	}
	
	// Getters
	string GetClassName() { return m_ClassName; }
	string GetDisplayName() 
	{ 
		// Se não tiver display name, retorna o classname
		if (m_DisplayName == "" || !m_DisplayName)
			return m_ClassName;
		return m_DisplayName; 
	}
	string GetCategory() { return m_Category; }
	int GetPrice() { return m_Price; }
	int GetCost() { return m_Cost; }
	int GetRestock() { return m_Restock; }
	int GetLifetime() { return m_Lifetime; }
	map<string, bool> GetFlags() { return m_Flags; }
	array<string> GetVariants() { return m_Variants; }
	map<string, string> GetAttachments() { return m_Attachments; }
	array<string> GetKeywords() { return m_Keywords; }
	array<string> GetDefaultAttachments() { return m_DefaultAttachments; }
	int GetBasePrice() { return m_BasePrice; }
	
	// Setters
	void SetClassName(string name) { m_ClassName = name; }
	void SetDisplayName(string name) { m_DisplayName = name; }
	void SetCategory(string category) { m_Category = category; }
	void SetPrice(int price) { m_Price = price; }
	void SetBasePrice(int basePrice) { m_BasePrice = basePrice; }
	void SetCost(int cost) { m_Cost = cost; }
	void SetRestock(int restock) { m_Restock = restock; }
	void SetLifetime(int lifetime) { m_Lifetime = lifetime; }
	void SetDefaultAttachments(array<string> attachments)
	{
		m_DefaultAttachments.Clear();
		if (!attachments)
			return;
		foreach (string att : attachments)
		{
			if (att && att != "")
				m_DefaultAttachments.Insert(att);
		}
	}
	
	// Verifica se item está disponível na loja/mercado
	bool IsAvailableInStore()
	{
		if (m_Flags.Contains("Market"))
			return m_Flags.Get("Market");
		return false;
	}
	
	// Preço de venda (50% do preço de compra)
	// NOTA: Este método usa valor hardcoded. Considere usar configuração do sistema.
	int GetSellPrice()
	{
		const int SELL_PERCENT = 50; // 50% do preço de compra
		int sellPrice = m_Price * SELL_PERCENT / 100;
		if (sellPrice < 1)
			sellPrice = 1; // Garantir mínimo de 1 para evitar preço zero
		return sellPrice;
	}
	
	bool ValidateData()
	{
		if (!m_ClassName || m_ClassName == "") {
			Print("[AskalStore] ERROR: Invalid item class name");
			return false;
		}
		if (m_Price < 0) {
			Print("[AskalStore] ERROR: Invalid price for item " + m_ClassName);
			return false;
		}
		return true;
	}
	
	// Debug
	void DebugPrint()
	{
		Print("[AskalStore] DEBUG ==================");
		Print("[AskalStore] Item: " + m_ClassName);
		Print("[AskalStore] DisplayName: " + GetDisplayName());
		Print("[AskalStore] Price: " + m_Price);
		Print("[AskalStore] Cost: " + m_Cost);
		Print("[AskalStore] Variants: " + m_Variants.Count());
		Print("[AskalStore] Available in Store: " + IsAvailableInStore());
		Print("[AskalStore] ========================");
	}
}

