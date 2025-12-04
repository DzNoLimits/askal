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
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] 🔍 Verificando área em " + pos + " (box: " + boxSize + ")");
		
		// Verificação simplificada: se posição está muito próxima de zero, considerar inválida
		if (pos[1] < -1000 || pos[1] > 10000)
		{
			if (s_Debug)
				Print("[AskalVehicleSpawn] ❌ Posição Y inválida: " + pos[1]);
			return false;
		}
		
		// Verificar colisão usando GetGame().GetObjectsAtPosition
		// Usar raio baseado no tamanho da box (maior dimensão)
		float radius = boxSize[0];
		if (boxSize[1] > radius)
			radius = boxSize[1];
		if (boxSize[2] > radius)
			radius = boxSize[2];
		
		// Adicionar margem de segurança
		radius = radius * 0.5 + 1.0;
		
		array<Object> objects = new array<Object>();
		GetGame().GetObjectsAtPosition(pos, radius, objects, NULL);
		
		if (objects && objects.Count() > 0)
		{
			// Filtrar objetos que não são obstáculos (itens pequenos, etc)
			int obstacleCount = 0;
			for (int i = 0; i < objects.Count(); i++)
			{
				Object obj = objects.Get(i);
				if (!obj)
					continue;
				
				// Verificar se é um objeto grande o suficiente para ser obstáculo
				// Usar bounding box ou aproximação baseada em tipo
				string objType = obj.GetType();
				if (objType && objType != "")
				{
					// Ignorar players verificando pelo tipo (sem usar PlayerBase que não está disponível em 3_Game)
					// Players geralmente têm tipos como "SurvivorM_*" ou "SurvivorF_*"
					string lowerType = objType;
					lowerType.ToLower();
					if (lowerType.IndexOf("survivor") != -1 || lowerType.IndexOf("player") != -1)
						continue;
					
					// Ignorar itens pequenos verificando se não é veículo e se está em CfgWeapons ou CfgMagazines
					// Itens pequenos geralmente estão em CfgWeapons ou CfgMagazines, não em CfgVehicles
					string weaponTest = "";
					GetGame().ConfigGetText("CfgWeapons " + objType + " displayName", weaponTest);
					if (weaponTest && weaponTest != "")
					{
						// É uma arma, ignorar (item pequeno)
						continue;
					}
					
					string magazineTest = "";
					GetGame().ConfigGetText("CfgMagazines " + objType + " displayName", magazineTest);
					if (magazineTest && magazineTest != "")
					{
						// É munição, ignorar (item pequeno)
						continue;
					}
					
					// Se não é veículo e não está em CfgWeapons/CfgMagazines, pode ser um item pequeno também
					// Verificar se está em CfgVehicles mas não é veículo (pode ser estrutura ou objeto grande)
					if (!IsVehicleClass(objType))
					{
						// Verificar se está em CfgVehicles (pode ser estrutura grande)
						string vehicleTest = "";
						GetGame().ConfigGetText("CfgVehicles " + objType + " displayName", vehicleTest);
						if (!vehicleTest || vehicleTest == "")
						{
							// Não está em CfgVehicles, provavelmente é item pequeno, ignorar
							continue;
						}
					}
					
					// Qualquer outro objeto grande (estruturas, veículos, etc) é considerado obstáculo
					obstacleCount++;
				}
			}
			
			if (obstacleCount > 0)
			{
				if (s_Debug)
					Print("[AskalVehicleSpawn] ❌ Área obstruída: " + obstacleCount + " obstáculo(s) encontrado(s)");
				return false;
			}
		}
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] ✅ Área livre");
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
	// Retorna o veículo criado (ou NULL se falhou)
	static Object SpawnVehicleAtPosition(string vehicleClass, vector pos, vector rotation, string ownerId = "")
	{
		if (!vehicleClass || vehicleClass == "")
		{
			Print("[AskalVehicleSpawn] ❌ Classe de veículo inválida");
			return NULL;
		}
		
		if (pos == vector.Zero)
		{
			Print("[AskalVehicleSpawn] ❌ Posição inválida");
			return NULL;
		}
		
		if (!GetGame().IsServer())
		{
			Print("[AskalVehicleSpawn] ❌ Spawn só pode ser feito no servidor");
			return NULL;
		}
		
		Print("[AskalVehicleSpawn] 🚗 Spawnando veículo: " + vehicleClass + " em " + pos + " (rotation: " + rotation + ")");
		
		// Criar veículo usando CreateObjectEx
		Object vehicle = GetGame().CreateObjectEx(vehicleClass, pos, ECE_PLACE_ON_SURFACE);
		
		if (!vehicle)
		{
			Print("[AskalVehicleSpawn] ❌ Falha ao criar veículo: " + vehicleClass);
			return NULL;
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
			return NULL;
		}
		
		// Log de sucesso
		string vehicleId = "";
		if (vehicle)
			vehicleId = vehicle.GetType();
		
		Print("[AskalVehicleSpawn] ✅ Veículo spawnado com sucesso: " + vehicleClass + " (ID: " + vehicleId + ") em " + pos);
		if (ownerId && ownerId != "")
			Print("[AskalVehicleSpawn]   Owner: " + ownerId);
		
		return vehicle;
	}
	
	// Verificar se uma classe é um veículo
	static bool IsVehicleClass(string className)
	{
		if (!className || className == "")
			return false;
		
		// Verificar se está em CfgVehicles (veículos estão em CfgVehicles)
		string testDisplayName = "";
		GetGame().ConfigGetText("CfgVehicles " + className + " displayName", testDisplayName);
		
		if (!testDisplayName || testDisplayName == "")
			return false; // Não está em CfgVehicles
		
		// Verificar se não é arma ou munição
		string weaponTest = "";
		GetGame().ConfigGetText("CfgWeapons " + className + " displayName", weaponTest);
		if (weaponTest && weaponTest != "")
			return false; // É arma, não veículo
		
		string magazineTest = "";
		GetGame().ConfigGetText("CfgMagazines " + className + " displayName", magazineTest);
		if (magazineTest && magazineTest != "")
			return false; // É munição, não veículo
		
		// Verificar por padrões no nome da classe PRIMEIRO (mais rápido)
		string lowerClassName = className;
		lowerClassName.ToLower();
		
		// Padrões comuns de veículos do DayZ (com underscore)
		if (lowerClassName.IndexOf("car_") == 0)
			return true;
		if (lowerClassName.IndexOf("truck_") == 0)
			return true;
		if (lowerClassName.IndexOf("boat_") == 0)
			return true;
		if (lowerClassName.IndexOf("bicycle_") == 0)
			return true;
		if (lowerClassName.IndexOf("offroad_") == 0)
			return true;
		if (lowerClassName.IndexOf("sedan_") == 0)
			return true;
		if (lowerClassName.IndexOf("hatchback_") == 0)
			return true;
		
		// Padrões sem underscore (ex: offroadhatchback, sedan_02, etc)
		if (lowerClassName.IndexOf("offroad") == 0)
			return true;
		if (lowerClassName.IndexOf("sedan") == 0)
			return true;
		if (lowerClassName.IndexOf("hatchback") == 0)
			return true;
		if (lowerClassName.IndexOf("truck") == 0)
			return true;
		if (lowerClassName.IndexOf("boat") == 0)
			return true;
		if (lowerClassName.IndexOf("bicycle") == 0)
			return true;
		if (lowerClassName.IndexOf("car") == 0)
			return true;
		
		// Padrões no meio ou fim do nome
		if (lowerClassName.IndexOf("_car") != -1)
			return true;
		if (lowerClassName.IndexOf("_truck") != -1)
			return true;
		if (lowerClassName.IndexOf("_boat") != -1)
			return true;
		
		// Verificar se herda de classes de veículos conhecidas
		// Verificar hierarquia de herança recursivamente
		string currentClass = className;
		int maxDepth = 20; // Limite de profundidade para evitar loops infinitos
		int depth = 0;
		
		while (currentClass && currentClass != "" && depth < maxDepth)
		{
			// Verificar se é uma classe de veículo conhecida
			string lowerClass = currentClass;
			lowerClass.ToLower();
			
			// Verificar classes base de veículos (uma por vez para compatibilidade)
			if (lowerClass == "car_base")
				return true;
			if (lowerClass == "car")
				return true;
			if (lowerClass == "truck_base")
				return true;
			if (lowerClass == "truck")
				return true;
			if (lowerClass == "boat_base")
				return true;
			if (lowerClass == "boat")
				return true;
			if (lowerClass == "bicyclebase")
				return true;
			if (lowerClass == "bicycle")
				return true;
			
			// Verificar se herda de ItemBase (se sim, é um item, não veículo)
			if (lowerClass == "itembase")
				return false;
			if (lowerClass == "item_base")
				return false;
			
			// Obter classe pai
			string parentClass = "";
			GetGame().ConfigGetText("CfgVehicles " + currentClass + " parent", parentClass);
			
			if (!parentClass || parentClass == "" || parentClass == currentClass)
				break; // Sem pai ou loop detectado
			
			currentClass = parentClass;
			depth++;
		}
		
		// Se chegou aqui, não encontrou evidências de ser veículo, assumir que não é
		return false;
	}
	
	// Verificar se veículo é terrestre (herda de Car_Base)
	static bool IsLandVehicle(string vehicleClass)
	{
		if (!vehicleClass || vehicleClass == "")
			return false;
		
		// Verificar herança na configuração
		string parentClass = "";
		GetGame().ConfigGetText("CfgVehicles " + vehicleClass + " parent", parentClass);
		
		// Verificar se herda de Car_Base ou classes relacionadas
		if (parentClass && parentClass != "")
		{
			if (parentClass == "Car_Base" || parentClass == "Car" || parentClass == "Truck_Base" || parentClass == "Truck")
				return true;
			
			// Verificar recursivamente na hierarquia
			if (IsLandVehicle(parentClass))
				return true;
		}
		
		// Verificar se o nome da classe contém indicadores de veículo terrestre
		string lowerClass = vehicleClass;
		lowerClass.ToLower();
		if (lowerClass.IndexOf("car") != -1 || lowerClass.IndexOf("truck") != -1 || lowerClass.IndexOf("van") != -1)
			return true;
		
		return false;
	}
	
	// Verificar se veículo é aquático (herda de Boat_Base)
	static bool IsWaterVehicle(string vehicleClass)
	{
		if (!vehicleClass || vehicleClass == "")
			return false;
		
		// Verificar herança na configuração
		string parentClass = "";
		GetGame().ConfigGetText("CfgVehicles " + vehicleClass + " parent", parentClass);
		
		// Verificar se herda de Boat_Base ou classes relacionadas
		if (parentClass && parentClass != "")
		{
			if (parentClass == "Boat_Base" || parentClass == "Boat")
				return true;
			
			// Verificar recursivamente na hierarquia
			if (IsWaterVehicle(parentClass))
				return true;
		}
		
		// Verificar se o nome da classe contém indicadores de veículo aquático
		string lowerClass = vehicleClass;
		lowerClass.ToLower();
		if (lowerClass.IndexOf("boat") != -1 || lowerClass.IndexOf("ship") != -1)
			return true;
		
		return false;
	}
	
	// Verificar se posição está em água
	static bool IsSurfaceWater(vector pos)
	{
		if (pos == vector.Zero)
			return false;
		
		// Usar DayZ API para verificar se superfície é água
		// DayZ tem GetGame().SurfaceIsWater() ou similar?
		// Por enquanto, usar aproximação baseada em altura Y
		// Se Y estiver abaixo de um threshold, assumir água
		
		// Verificar usando raycast ou API de superfície se disponível
		// Por enquanto, usar aproximação conservadora
		// Água geralmente está em Y < 0 ou próximo de 0 em mapas padrão
		
		// TODO: Implementar verificação real usando engine API
		// Por enquanto, assumir que se Y < 1.0, pode ser água
		// Esta é uma aproximação que pode ser melhorada
		
		if (s_Debug)
			Print("[AskalVehicleSpawn] 💧 Verificando se superfície é água em " + pos);
		
		// Aproximação: se Y < 1.0, considerar possível água
		// Esta verificação deve ser melhorada com API real
		return pos[1] < 1.0;
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

