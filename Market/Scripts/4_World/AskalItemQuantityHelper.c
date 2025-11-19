// ==========================================
// AskalItemQuantityHelper - Helper para detectar e gerenciar tipos de quantidade de itens
// Baseado na lógica do COT (Community Online Tools)
// ==========================================
// NOTA: O enum AskalItemQuantityType está definido em AskalDatabaseStructures.c

class AskalItemQuantityHelper
{
	// Detectar tipo de quantidade de um item
	// Retorna: NONE, MAGAZINE, STACKABLE, ou QUANTIFIABLE
	static AskalItemQuantityType DetectQuantityType(string className)
	{
		if (!className || className == "")
			return AskalItemQuantityType.NONE;
		
		// Criar objeto temporário para inspeção
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return AskalItemQuantityType.NONE;
		
		ItemBase item = ItemBase.Cast(tempObj);
		if (!item)
		{
			GetGame().ObjectDelete(tempObj);
			return AskalItemQuantityType.NONE;
		}
		
		AskalItemQuantityType result = AskalItemQuantityType.NONE;
		
		// PRIORIDADE 1: Verificar se é Magazine (carregador de arma)
		Magazine mag = Magazine.Cast(item);
		if (mag)
		{
			int ammoMax = mag.GetAmmoMax();
			bool isSplitable = item.IsSplitable();
			bool hasQuantity = item.HasQuantity();
			
			// DIFERENCIAÇÃO CRÍTICA: Munições soltas (Ammunition_Base) vs Carregadores de arma
			// Munições soltas têm prefixo "Ammo" e são IsSplitable()
			// Carregadores de arma têm GetAmmoMax() > 0 e não são splitable
			
			// Verificar se é munição solta (stackable) primeiro
			if (isSplitable || (className.IndexOf("Ammo_") == 0 && hasQuantity))
			{
				// É munição solta (Ammunition_Base) - STACKABLE
				result = AskalItemQuantityType.STACKABLE;
				GetGame().ObjectDelete(tempObj);
				return result;
			}
			
			// Verificar se tem capacidade de munição (carregador de arma)
			if (ammoMax > 0)
			{
				// É um carregador de arma (MAGAZINE)
				result = AskalItemQuantityType.MAGAZINE;
				GetGame().ObjectDelete(tempObj);
				return result;
			}
			
			// Magazine sem ammoMax > 0 - provavelmente munição solta sem IsSplitable() detectado
			// Tratar como STACKABLE por segurança se tem quantidade
			if (hasQuantity)
			{
				result = AskalItemQuantityType.STACKABLE;
				GetGame().ObjectDelete(tempObj);
				return result;
			}
		}
		
		// PRIORIDADE 2: Verificar se é container de líquido (QUANTIFIABLE)
		if (item.IsLiquidContainer())
		{
			result = AskalItemQuantityType.QUANTIFIABLE;
			GetGame().ObjectDelete(tempObj);
			return result;
		}
		
		// PRIORIDADE 3: Verificar se é um item com quantidade (STACKABLE)
		if (item.HasQuantity())
		{
			float qtyMin = item.GetQuantityMin();
			float qtyMax = item.GetQuantityMax();
			
			// Verifica se tem range de quantidade válido
			if (qtyMax - qtyMin > 0)
			{
				// STACKABLE: Itens que podem ser divididos (split)
				// Inclui munição solta, pregos, tábuas, etc automaticamente
				if (item.IsSplitable())
				{
					result = AskalItemQuantityType.STACKABLE;
				}
				// Fracionáveis genéricos (sem split) ficam como QUANTIFIABLE
				else
				{
					result = AskalItemQuantityType.QUANTIFIABLE;
				}
			}
		}
		
		GetGame().ObjectDelete(tempObj);
		return result;
	}
	
	// Obter valores min/max para stackables baseado no config e no objeto
	// Retorna valores corretos para o slider
	static void GetStackableRange(string className, out int minValue, out int maxValue, out float stepValue)
	{
		minValue = 1;
		maxValue = 50; // Fallback padrão
		stepValue = 1.0;
		
		// Criar objeto temporário para inspeção
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return;
		
		ItemBase item = ItemBase.Cast(tempObj);
		if (!item || !item.HasQuantity())
		{
			GetGame().ObjectDelete(tempObj);
			return;
		}
		
		// Ler valores do objeto (mais confiável que config)
		float qtyMin = item.GetQuantityMin();
		float qtyMax = item.GetQuantityMax();
		
		// Verificar se é munição (pode ter count no config)
		if (className.IndexOf("Ammo_") == 0)
		{
			// Tentar ler count do config CfgMagazines
			string configPath = "CfgMagazines " + className + " count";
			if (GetGame().ConfigIsExisting(configPath))
			{
				int configCount = GetGame().ConfigGetInt(configPath);
				if (configCount > 0)
				{
					maxValue = configCount;
					GetGame().ObjectDelete(tempObj);
					return;
				}
			}
		}
		
		// Para outros stackables, tentar ler varQuantityMax do config CfgVehicles
		// Exemplo: Nail tem varQuantityMax = 99.0
		string varQtyMaxPath = "CfgVehicles " + className + " varQuantityMax";
		if (GetGame().ConfigIsExisting(varQtyMaxPath))
		{
			float configVarQtyMax = GetGame().ConfigGetFloat(varQtyMaxPath);
			if (configVarQtyMax > 0)
			{
				maxValue = Math.Round(configVarQtyMax);
				// Ler também varQuantityMin se existir
				string varQtyMinPath = "CfgVehicles " + className + " varQuantityMin";
				if (GetGame().ConfigIsExisting(varQtyMinPath))
				{
					float configVarQtyMin = GetGame().ConfigGetFloat(varQtyMinPath);
					if (configVarQtyMin >= 0)
					{
						minValue = Math.Round(configVarQtyMin);
					}
				}
				GetGame().ObjectDelete(tempObj);
				return;
			}
		}
		
		// Fallback: usar valores do objeto
		if (qtyMax > 0)
		{
			maxValue = Math.Round(qtyMax);
		}
		
		// Para AmmoPile, mínimo é 1 se max > 1
		if (className.IndexOf("Ammo_") == 0 && maxValue > 1)
		{
			Magazine mag = Magazine.Cast(item);
			if (mag && mag.IsAmmoPile())
			{
				minValue = 1;
			}
		}
		
		// Step value baseado em IsSplitable
		if (item.IsSplitable())
		{
			stepValue = 1.0; // Valores inteiros
		}
		else
		{
			stepValue = 0.1; // Valores fracionários
		}
		
		GetGame().ObjectDelete(tempObj);
	}
	
	// Obter valores min/max para magazines (carregadores)
	static void GetMagazineRange(string className, out int minValue, out int maxValue, out float stepValue)
	{
		minValue = 0;
		maxValue = 30; // Fallback padrão
		stepValue = 1.0;
		
		// Criar objeto temporário para inspeção
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return;
		
		Magazine mag = Magazine.Cast(tempObj);
		if (!mag)
		{
			GetGame().ObjectDelete(tempObj);
			return;
		}
		
		int ammoMax = mag.GetAmmoMax();
		if (ammoMax <= 0)
			ammoMax = 1;
		
		// Para AmmoPile, mínimo é 1 se max > 1
		if (mag.IsAmmoPile() && ammoMax > 1)
		{
			minValue = 1;
		}
		else
		{
			minValue = 0;
		}
		
		maxValue = ammoMax;
		stepValue = 1.0;
		
		GetGame().ObjectDelete(tempObj);
	}
	
	// Obter valores min/max para quantifiables (líquidos, bandagens, etc)
	static void GetQuantifiableRange(string className, out int minValue, out int maxValue, out float stepValue)
	{
		minValue = 0;
		maxValue = 100; // Percentual padrão
		stepValue = 1.0;
		
		// Criar objeto temporário para inspeção
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return;
		
		ItemBase item = ItemBase.Cast(tempObj);
		if (!item || !item.HasQuantity())
		{
			GetGame().ObjectDelete(tempObj);
			return;
		}
		
		// Para quantifiables, sempre usar percentual (0-100)
		// Step value baseado em IsSplitable
		if (item.IsSplitable())
		{
			stepValue = 1.0; // Valores inteiros
		}
		else
		{
			stepValue = 0.1; // Valores fracionários
		}
		
		GetGame().ObjectDelete(tempObj);
	}
	
	// Verificar se item é munição solta (stackable)
	static bool IsAmmunition(string className)
	{
		if (!className || className == "")
			return false;
		
		// Munições têm prefixo "Ammo_"
		return className.IndexOf("Ammo_") == 0;
	}
	
	// Verificar se item é carregador de arma (magazine)
	static bool IsMagazine(string className)
	{
		if (!className || className == "")
			return false;
		
		// Criar objeto temporário para inspeção
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return false;
		
		Magazine mag = Magazine.Cast(tempObj);
		if (!mag)
		{
			GetGame().ObjectDelete(tempObj);
			return false;
		}
		
		// É magazine se tem GetAmmoMax() > 0 e não é splitable
		int ammoMax = mag.GetAmmoMax();
		ItemBase item = ItemBase.Cast(tempObj);
		bool isSplitable = item && item.IsSplitable();
		
		GetGame().ObjectDelete(tempObj);
		
		// É carregador se tem ammoMax > 0 e não é splitable
		return (ammoMax > 0 && !isSplitable);
	}
}

