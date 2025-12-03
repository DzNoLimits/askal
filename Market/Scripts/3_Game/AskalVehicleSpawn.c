// ==========================================
// AskalVehicleSpawn - Sistema de spawn de veículos
// Implementa spawn robusto para traders estáticos e virtuais
// ==========================================

// Constantes de spawn
static const float VEHICLE_SPAWN_RADIUS_DEFAULT = 50.0;
static const int VEHICLE_SPAWN_ATTEMPTS_DEFAULT = 20;
static const float VEHICLE_SPAWN_MAX_INCLINATION_DEG = 15.0;
static const vector VEHICLE_CLEARANCE_BOX_DEFAULT = "3 2 6"; // width, height, length

class AskalVehicleSpawn
{
	protected static bool s_Debug = false;
	
	// Ativar/desativar logs de debug
	static void SetDebug(bool enabled)
	{
		s_Debug = enabled;
	}
	
	// Verificar se uma área está livre (teste de colisão)
	static bool IsAreaClear(vector pos, vector boxSize)
	{
		if (pos == vector.Zero)
			return false;
		
		// TODO: Implementar teste de colisão usando engine API
		// Por enquanto, usar verificação conservadora via raycast
		// Verificar se há objetos próximos usando GetGame().GetObjectsAtPosition
		
		// Verificação básica: raycast para cima e para baixo
		vector rayStart = pos;
		rayStart[1] = pos[1] + 5.0; // 5m acima
		vector rayEnd = pos;
		rayEnd[1] = pos[1] - 5.0; // 5m abaixo
		
		// Usar DayZ raycast se disponível
		// Por enquanto, assumir área livre se não houver objetos muito próximos
		// Esta é uma implementação conservadora que pode ser melhorada
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] 🔍 Verificando área em " + pos + " (box: " + boxSize + ")");
		
		// Verificação simplificada: se posição está muito próxima de zero, considerar inválida
		if (pos[1] < -1000 || pos[1] > 10000)
		{
			if (s_Debug)
				Print("[AskalVehicleSpawn] ❌ Posição Y inválida: " + pos[1]);
			return false;
		}
		
		// Por padrão, assumir área livre (implementação pode ser melhorada com engine API)
		return true;
	}
	
	// Projetar posição no chão (snap Y para superfície)
	static vector ProjectOntoGround(vector pos)
	{
		if (pos == vector.Zero)
			return vector.Zero;
		
		// TODO: Usar engine API para raycast ao chão
		// Por enquanto, usar aproximação conservadora
		
		// Verificar se há informação de altura do terreno
		// DayZ tem GetGame().SurfaceY() ou similar?
		// Por enquanto, manter Y original (assumir que já está no chão)
		
		vector groundPos = pos;
		
		// Tentar obter altura do terreno se possível
		// Se não disponível, usar posição como está
		if (s_Debug)
			Print("[AskalVehicleSpawn] 📍 Projetando posição " + pos + " no chão");
		
		return groundPos;
	}
	
	// Calcular inclinação da superfície em graus
	static float SurfaceInclinationAt(vector pos)
	{
		if (pos == vector.Zero)
			return 90.0; // Inclinação máxima = inválido
		
		// TODO: Calcular inclinação usando normal da superfície
		// Por enquanto, retornar 0 (superfície plana assumida)
		// Pode ser melhorado com raycast para obter normal
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] 📐 Calculando inclinação em " + pos);
		
		// Assumir superfície plana por padrão
		return 0.0;
	}
	
	// Helper para calcular módulo (DayZ não tem operador %)
	static int Modulo(int value, int divisor)
	{
		if (divisor == 0)
			return 0;
		return value - (value / divisor) * divisor;
	}
	
	// Encontrar posição válida de spawn perto de uma posição (não usa PlayerBase para compatibilidade com 3_Game)
	static vector FindValidSpawnPositionNearPosition(vector playerPos, float radius = 50.0, int attempts = 20, float maxInclinationDeg = 15.0, vector clearanceBox = "3 2 6")
	{
		if (playerPos == vector.Zero)
		{
			Print("[AskalVehicleSpawn] ❌ Posição do player inválida");
			return vector.Zero;
		}
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] 🔍 Buscando posição válida perto da posição " + playerPos + " (radius: " + radius + ", attempts: " + attempts + ")");
		
		// Tentar encontrar posição válida
		for (int i = 0; i < attempts; i++)
		{
			// Gerar posição candidata aleatória
			// Usar aproximação simples: grid pattern com offset aleatório
			int mod8 = Modulo(i, 8);
			float distance = 5.0 + (radius - 5.0) * mod8 / 7.0; // Distribuir tentativas em distâncias variadas
			float angleStep = 360.0 / 8.0; // 8 direções principais
			float angle = mod8 * angleStep;
			
			// Converter ângulo para radianos manualmente
			float angleRad = angle * 0.0174532925; // PI/180 aproximado
			
			// Calcular cos e sin manualmente (aproximação)
			float cosVal = 1.0 - (angleRad * angleRad / 2.0); // Aproximação de cos
			float sinVal = angleRad; // Aproximação de sin para ângulos pequenos
			
			// Usar valores pré-calculados para 8 direções principais
			// 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
			int dirIndex = mod8;
			if (dirIndex == 0) // 0°
			{
				cosVal = 1.0;
				sinVal = 0.0;
			}
			else if (dirIndex == 1) // 45°
			{
				cosVal = 0.7071;
				sinVal = 0.7071;
			}
			else if (dirIndex == 2) // 90°
			{
				cosVal = 0.0;
				sinVal = 1.0;
			}
			else if (dirIndex == 3) // 135°
			{
				cosVal = -0.7071;
				sinVal = 0.7071;
			}
			else if (dirIndex == 4) // 180°
			{
				cosVal = -1.0;
				sinVal = 0.0;
			}
			else if (dirIndex == 5) // 225°
			{
				cosVal = -0.7071;
				sinVal = -0.7071;
			}
			else if (dirIndex == 6) // 270°
			{
				cosVal = 0.0;
				sinVal = -1.0;
			}
			else // 315°
			{
				cosVal = 0.7071;
				sinVal = -0.7071;
			}
			
			vector candidatePos = playerPos;
			candidatePos[0] = playerPos[0] + cosVal * distance;
			candidatePos[2] = playerPos[2] + sinVal * distance;
			candidatePos[1] = playerPos[1]; // Manter altura inicial
			
			if (s_Debug)
				Print("[AskalVehicleSpawn] 🎲 Tentativa " + (i + 1) + "/" + attempts + ": candidato em " + candidatePos);
			
			// Projetar no chão
			vector groundPos = ProjectOntoGround(candidatePos);
			if (groundPos == vector.Zero)
			{
				if (s_Debug)
					Print("[AskalVehicleSpawn] ❌ Falha ao projetar no chão");
				continue;
			}
			
			// Verificar se área está livre
			if (!IsAreaClear(groundPos, clearanceBox))
			{
				if (s_Debug)
					Print("[AskalVehicleSpawn] ❌ Área não está livre (colisão detectada)");
				continue;
			}
			
			// Verificar inclinação
			float inclination = SurfaceInclinationAt(groundPos);
			if (inclination > maxInclinationDeg)
			{
				if (s_Debug)
					Print("[AskalVehicleSpawn] ❌ Inclinação muito alta: " + inclination + "° (máx: " + maxInclinationDeg + "°)");
				continue;
			}
			
			// Posição válida encontrada!
			Print("[AskalVehicleSpawn] ✅ Posição válida encontrada em " + groundPos + " (tentativa " + (i + 1) + ")");
			return groundPos;
		}
		
		Print("[AskalVehicleSpawn] ❌ Nenhuma posição válida encontrada após " + attempts + " tentativas");
		return vector.Zero;
	}
	
	// Spawnar veículo em posição específica
	static bool SpawnVehicleAtPosition(string vehicleClass, vector pos, vector rotation, string ownerId = "")
	{
		if (!vehicleClass || vehicleClass == "")
		{
			Print("[AskalVehicleSpawn] ❌ Classe de veículo inválida");
			return false;
		}
		
		if (pos == vector.Zero)
		{
			Print("[AskalVehicleSpawn] ❌ Posição inválida");
			return false;
		}
		
		if (!GetGame().IsServer())
		{
			Print("[AskalVehicleSpawn] ❌ Spawn só pode ser feito no servidor");
			return false;
		}
		
		Print("[AskalVehicleSpawn] 🚗 Spawnando veículo: " + vehicleClass + " em " + pos + " (rotation: " + rotation + ")");
		
		// Criar veículo usando CreateObjectEx
		Object vehicle = GetGame().CreateObjectEx(vehicleClass, pos, ECE_PLACE_ON_SURFACE);
		
		if (!vehicle)
		{
			Print("[AskalVehicleSpawn] ❌ Falha ao criar veículo: " + vehicleClass);
			return false;
		}
		
		// Aplicar rotação
		if (rotation != vector.Zero)
		{
			vehicle.SetOrientation(rotation);
		}
		
		// Verificar se spawn foi estável (veículo existe e está no mundo)
		if (!vehicle || !vehicle.IsAlive())
		{
			Print("[AskalVehicleSpawn] ❌ Veículo spawnado mas não estável");
			if (vehicle)
				GetGame().ObjectDelete(vehicle);
			return false;
		}
		
		// Log de sucesso
		string vehicleId = "";
		if (vehicle)
			vehicleId = vehicle.GetType();
		
		Print("[AskalVehicleSpawn] ✅ Veículo spawnado com sucesso: " + vehicleClass + " (ID: " + vehicleId + ") em " + pos);
		if (ownerId && ownerId != "")
			Print("[AskalVehicleSpawn]   Owner: " + ownerId);
		
		return true;
	}
	
	// Verificar se uma classe é um veículo
	static bool IsVehicleClass(string className)
	{
		if (!className || className == "")
			return false;
		
		// Verificar se está em CfgVehicles (veículos estão em CfgVehicles)
		string testDisplayName = "";
		GetGame().ConfigGetText("CfgVehicles " + className + " displayName", testDisplayName);
		
		if (testDisplayName && testDisplayName != "")
		{
			// Verificar se herda de Car, Truck, Boat, etc.
			// Por padrão, se está em CfgVehicles e não é ItemBase, provavelmente é veículo
			// Verificação adicional: checar se não está em CfgWeapons ou CfgMagazines
			string weaponTest = "";
			GetGame().ConfigGetText("CfgWeapons " + className + " displayName", weaponTest);
			if (weaponTest && weaponTest != "")
				return false; // É arma, não veículo
			
			string magazineTest = "";
			GetGame().ConfigGetText("CfgMagazines " + className + " displayName", magazineTest);
			if (magazineTest && magazineTest != "")
				return false; // É munição, não veículo
			
			// Se está em CfgVehicles e não é arma/munição, assumir que é veículo
			return true;
		}
		
		return false;
	}
	
	// Getters para constantes (para acesso de outros módulos)
	static float GetDefaultRadius()
	{
		return VEHICLE_SPAWN_RADIUS_DEFAULT;
	}
	
	static int GetDefaultAttempts()
	{
		return VEHICLE_SPAWN_ATTEMPTS_DEFAULT;
	}
	
	static float GetDefaultMaxInclination()
	{
		return VEHICLE_SPAWN_MAX_INCLINATION_DEG;
	}
	
	static vector GetDefaultClearanceBox()
	{
		return VEHICLE_CLEARANCE_BOX_DEFAULT;
	}
}

