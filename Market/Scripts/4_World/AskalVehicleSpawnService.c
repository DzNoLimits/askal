// ==========================================
// AskalVehicleSpawnService - Serviço de spawn de veículos
// Detecta CarScript vs BoatScript e spawna em posições apropriadas
// ==========================================

class AskalVehicleSpawnService
{
	// Verificar se uma classe é um veículo (CarScript ou BoatScript)
	static bool IsVehicle(string className)
	{
		if (!className || className == "")
			return false;
		
		Print("[AskalVehicleSpawn] 🔍 Verificando se é veículo: " + className);
		
		// Criar objeto temporário para verificar tipo
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
		{
			Print("[AskalVehicleSpawn] ⚠️ Não foi possível criar objeto temporário: " + className);
			return false;
		}
		
		CarScript car = CarScript.Cast(tempObj);
		BoatScript boat = BoatScript.Cast(tempObj);
		
		bool isVehicle = (car != NULL || boat != NULL);
		
		if (isVehicle)
			Print("[AskalVehicleSpawn] ✅ É veículo: " + className + " (Car: " + (car != NULL).ToString() + ", Boat: " + (boat != NULL).ToString() + ")");
		else
			Print("[AskalVehicleSpawn] ❌ Não é veículo: " + className);
		
		GetGame().ObjectDelete(tempObj);
		return isVehicle;
	}
	
	// Verificar se é CarScript (veículo terrestre)
	static bool IsCarScript(string className)
	{
		if (!className || className == "")
			return false;
		
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return false;
		
		CarScript car = CarScript.Cast(tempObj);
		bool isCar = (car != NULL);
		
		GetGame().ObjectDelete(tempObj);
		return isCar;
	}
	
	// Verificar se é BoatScript (veículo aquático)
	static bool IsBoatScript(string className)
	{
		if (!className || className == "")
			return false;
		
		Object tempObj = GetGame().CreateObject(className, vector.Zero, true, false, false);
		if (!tempObj)
			return false;
		
		BoatScript boat = BoatScript.Cast(tempObj);
		bool isBoat = (boat != NULL);
		
		GetGame().ObjectDelete(tempObj);
		return isBoat;
	}
	
	// Spawnar veículo no mundo (não no inventário)
	// Retorna o veículo spawnado ou NULL se falhar
	static EntityAI SpawnVehicleInWorld(PlayerBase player, string vehicleClass, string traderName = "")
	{
		if (!player || !vehicleClass || vehicleClass == "")
		{
			Print("[AskalVehicleSpawn] ❌ Parâmetros inválidos");
			return NULL;
		}
		
		if (!GetGame().IsServer())
		{
			Print("[AskalVehicleSpawn] ❌ SpawnVehicleInWorld só pode ser chamado no servidor");
			return NULL;
		}
		
		// Verificar se é veículo
		if (!IsVehicle(vehicleClass))
		{
			Print("[AskalVehicleSpawn] ❌ Classe não é um veículo: " + vehicleClass);
			return NULL;
		}
		
		// Obter posição e orientação de spawn apropriadas
		vector spawnPosition = vector.Zero;
		vector spawnOrientation = vector.Zero;
		
		// Tentar obter de pontos configurados primeiro
		if (traderName != "")
		{
			Print("[AskalVehicleSpawn] 🔍 Procurando pontos de spawn configurados para trader: " + traderName);
			AskalTraderConfig traderConfig = AskalTraderConfig.LoadByTraderName(traderName);
			if (traderConfig)
			{
				Print("[AskalVehicleSpawn] ✅ Trader config encontrado: " + traderName);
				
				// Determinar tipo de spawn (Land ou Water)
				string spawnType = "Land";
				if (IsBoatScript(vehicleClass))
					spawnType = "Water";
				
				Print("[AskalVehicleSpawn] 📍 Tipo de spawn: " + spawnType);
				
				// Tentar usar pontos de spawn configurados
				array<ref VehicleSpawnPoint> spawnPoints = traderConfig.GetVehicleSpawnPointsForType(spawnType);
				string spawnPointsCount = "0";
				if (spawnPoints)
					spawnPointsCount = spawnPoints.Count().ToString();
				Print("[AskalVehicleSpawn] 📍 Pontos de spawn encontrados: " + spawnPointsCount);
				
				if (spawnPoints && spawnPoints.Count() > 0)
				{
					// Tentar cada ponto de spawn configurado
					for (int i = 0; i < spawnPoints.Count(); i++)
					{
						VehicleSpawnPoint spawnPoint = spawnPoints.Get(i);
						if (!spawnPoint)
						{
							Print("[AskalVehicleSpawn] ⚠️ Ponto de spawn #" + (i + 1).ToString() + " é NULL");
							continue;
						}
						
						vector pointPos = spawnPoint.GetPosition();
						Print("[AskalVehicleSpawn] 📍 Ponto de spawn #" + (i + 1).ToString() + " posição lida: " + pointPos.ToString());
						Print("[AskalVehicleSpawn] 📍 Componentes: X=" + pointPos[0].ToString() + " Y=" + pointPos[1].ToString() + " Z=" + pointPos[2].ToString());
						
						if (pointPos == vector.Zero)
						{
							Print("[AskalVehicleSpawn] ⚠️ Ponto de spawn #" + (i + 1).ToString() + " tem posição zero");
							continue;
						}
						
						// Verificar se coordenadas são válidas (não extremos do mapa)
						float posX = pointPos[0];
						float posZ = pointPos[2];
						bool xInvalid = (posX < -100000 || posX > 100000);
						bool zInvalid = (posZ < -100000 || posZ > 100000);
						if (xInvalid || zInvalid)
						{
							Print("[AskalVehicleSpawn] ⚠️ Ponto de spawn #" + (i + 1).ToString() + " tem coordenadas inválidas (extremidades do mapa)");
							continue;
						}
						
						// Para pontos configurados, usar verificação menos restritiva (raio menor)
						// ou pular verificação se necessário
						bool isFree = IsSpawnPositionFree(pointPos, vehicleClass, 2.0);
						Print("[AskalVehicleSpawn] 📍 Ponto de spawn #" + (i + 1).ToString() + " está livre: " + isFree.ToString());
						
						if (isFree)
						{
							spawnPosition = pointPos;
							spawnOrientation = spawnPoint.GetRotation();
							Print("[AskalVehicleSpawn] ✅ Usando ponto de spawn configurado #" + (i + 1).ToString() + " do trader: " + traderName);
							break;
						}
						else
						{
							Print("[AskalVehicleSpawn] ⚠️ Ponto de spawn #" + (i + 1).ToString() + " ocupado, tentando próximo...");
						}
					}
					
					// Se nenhum ponto configurado está livre, usar o primeiro mesmo assim (forçar spawn)
					if (spawnPosition == vector.Zero && spawnPoints.Count() > 0)
					{
						VehicleSpawnPoint firstPoint = spawnPoints.Get(0);
						if (firstPoint)
						{
							vector firstPos = firstPoint.GetPosition();
							if (firstPos != vector.Zero)
							{
								Print("[AskalVehicleSpawn] ⚠️ Todos os pontos ocupados, forçando spawn no primeiro ponto");
								spawnPosition = firstPos;
								spawnOrientation = firstPoint.GetRotation();
							}
						}
					}
				}
				else
				{
					Print("[AskalVehicleSpawn] ⚠️ Nenhum ponto de spawn configurado encontrado para tipo: " + spawnType);
				}
			}
			else
			{
				Print("[AskalVehicleSpawn] ⚠️ Trader config não encontrado: " + traderName);
			}
		}
		
		// Se não encontrou ponto configurado, usar método padrão
		if (spawnPosition == vector.Zero)
		{
			spawnPosition = GetVehicleSpawnPosition(player, vehicleClass, traderName);
			if (spawnPosition == vector.Zero)
			{
				Print("[AskalVehicleSpawn] ❌ Não foi possível determinar posição de spawn");
				return NULL;
			}
			
			// Obter orientação de spawn padrão
			spawnOrientation = GetVehicleSpawnOrientation(player, vehicleClass, traderName);
		}
		
		Print("[AskalVehicleSpawn] 🚗 Spawnando veículo: " + vehicleClass);
		Print("[AskalVehicleSpawn]   Posição: " + spawnPosition.ToString());
		Print("[AskalVehicleSpawn]   Orientação: " + spawnOrientation.ToString());
		Print("[AskalVehicleSpawn]   Componentes da posição: X=" + spawnPosition[0].ToString() + " Y=" + spawnPosition[1].ToString() + " Z=" + spawnPosition[2].ToString());
		
		// Criar veículo no mundo
		// Usar flags similares ao TraderX: ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS
		// NÃO usar ECE_PLACE_ON_SURFACE para veículos, pois pode colocar em posição errada
		EntityAI vehicle = EntityAI.Cast(GetGame().CreateObjectEx(vehicleClass, spawnPosition, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE));
		if (!vehicle)
		{
			Print("[AskalVehicleSpawn] ❌ Falha ao criar veículo: " + vehicleClass);
			return NULL;
		}
		
		// Sincronizar com clientes (similar ao Trader)
		GetGame().RemoteObjectCreate(vehicle);
		
		// Verificar posição antes de definir
		vector vehiclePosBefore = vehicle.GetPosition();
		Print("[AskalVehicleSpawn] 📍 Posição do veículo ANTES de SetPosition: " + vehiclePosBefore.ToString());
		
		// Configurar posição e orientação
		vehicle.SetPosition(spawnPosition);
		vehicle.SetOrientation(spawnOrientation);
		
		// Verificar posição depois de definir
		vector vehiclePosAfter = vehicle.GetPosition();
		Print("[AskalVehicleSpawn] 📍 Posição do veículo DEPOIS de SetPosition: " + vehiclePosAfter.ToString());
		Print("[AskalVehicleSpawn] 📍 Componentes após SetPosition: X=" + vehiclePosAfter[0].ToString() + " Y=" + vehiclePosAfter[1].ToString() + " Z=" + vehiclePosAfter[2].ToString());
		
		// Aplicar direção baseada na orientação
		// Converter orientação (yaw, pitch, roll) para direção
		float yaw = spawnOrientation[0];
		float pitch = spawnOrientation[1];
		float roll = spawnOrientation[2];
		
		// Calcular direção a partir do yaw (rotação Y)
		// Yaw em graus, converter para radianos se necessário
		// Por enquanto, usar orientação diretamente
		vehicle.SetDirection(spawnOrientation);
		
		// Configurar veículo (fluidos, chaves, etc)
		ConfigureVehicle(vehicle, vehicleClass);
		
		Print("[AskalVehicleSpawn] ✅ Veículo spawnado com sucesso: " + vehicleClass);
		return vehicle;
	}
	
	// Obter posição de spawn do veículo
	// Para CarScript: usar posição "Land" (próximo ao player ou trader)
	// Para BoatScript: usar posição "Water" (próximo à água)
	static vector GetVehicleSpawnPosition(PlayerBase player, string vehicleClass, string traderName = "")
	{
		if (!player)
			return vector.Zero;
		
		vector playerPos = player.GetPosition();
		vector spawnPos = vector.Zero;
		
		// Se há trader configurado, tentar usar posição do trader com offset
		// (Pontos configurados são tratados em SpawnVehicleInWorld)
		if (traderName != "")
		{
			AskalTraderConfig traderConfig = AskalTraderConfig.LoadByTraderName(traderName);
			if (traderConfig)
			{
				vector traderPos = traderConfig.GetPosition();
				vector traderOrientation = traderConfig.GetOrientation();
				if (traderPos != vector.Zero)
				{
					// Verificar se posição do trader está livre (raio de 3m)
					spawnPos = CalculateSpawnOffset(traderPos, traderOrientation, vehicleClass);
					if (IsSpawnPositionFree(spawnPos, vehicleClass, 3.0))
					{
						Print("[AskalVehicleSpawn] 📍 Usando posição do trader: " + traderName);
						return spawnPos;
					}
					else
					{
						Print("[AskalVehicleSpawn] ⚠️ Posição do trader ocupada, procurando alternativa...");
					}
				}
			}
		}
		
		// Se não há trader configurado ou posição do trader está ocupada,
		// procurar área livre em raio de 50 metros do player
		// Raio de busca: 50m, Raio de área livre necessária: 3m
		Print("[AskalVehicleSpawn] 🔍 Procurando área livre para spawn (raio de busca: 50m, área livre: 3m)...");
		spawnPos = FindFreeSpawnArea(player, vehicleClass, 50.0, 3.0);
		
		if (spawnPos == vector.Zero)
		{
			Print("[AskalVehicleSpawn] ❌ Nenhuma área livre encontrada para spawn");
			return vector.Zero;
		}
		
		Print("[AskalVehicleSpawn] 📍 Área livre encontrada: " + spawnPos.ToString());
		return spawnPos;
	}
	
	// Calcular offset de spawn baseado no tipo de veículo e direção
	static vector CalculateSpawnOffset(vector basePosition, vector direction, string vehicleClass)
	{
		if (basePosition == vector.Zero)
			return vector.Zero;
		
		// Offset padrão: 5 metros à frente e 2 metros à direita
		// Usar direção do player/trader para calcular offset relativo
		float distanceForward = 5.0; // metros à frente
		float distanceRight = 2.0; // metros à direita
		
		// Normalizar direção para obter vetor unitário
		float dirLength = direction.Length();
		if (dirLength > 0.001)
		{
			float dirNormX = direction[0] / dirLength;
			float dirNormY = direction[1] / dirLength;
			float dirNormZ = direction[2] / dirLength;
			vector dirNormalized = Vector(dirNormX, dirNormY, dirNormZ);
			
			// Calcular offset relativo à direção
			// Forward: na direção do player (usar componente X e Z da direção)
			float forwardX = dirNormalized[0] * distanceForward;
			float forwardZ = dirNormalized[2] * distanceForward;
			
			// Right: perpendicular à direção (90 graus à direita)
			// Para obter perpendicular: trocar X e Z e inverter um deles
			float rightX = -dirNormalized[2] * distanceRight;
			float rightZ = dirNormalized[0] * distanceRight;
			
			vector offset = Vector(forwardX + rightX, 0.0, forwardZ + rightZ);
			
			// Para CarScript: spawnar em terra (mesma altura do player)
			if (IsCarScript(vehicleClass))
			{
				// Manter altura Y do player/trader
				return basePosition + offset;
			}
			// Para BoatScript: spawnar na água (altura Y ajustada)
			else if (IsBoatScript(vehicleClass))
			{
				// Para barcos, usar altura do player (será ajustado pelo jogo se necessário)
				// TODO: Implementar detecção de água se necessário
				return basePosition + offset;
			}
			
			// Fallback: offset padrão
			return basePosition + offset;
		}
		
		// Se direção inválida, usar offset fixo
		vector defaultOffset = Vector(5.0, 0.0, 2.0);
		return basePosition + defaultOffset;
	}
	
	// Obter orientação de spawn do veículo
	static vector GetVehicleSpawnOrientation(PlayerBase player, string vehicleClass, string traderName = "")
	{
		// Orientação padrão: mesmo que o player ou trader
		if (player)
		{
			vector playerDir = player.GetDirection();
			return playerDir;
		}
		
		// Se há trader, usar orientação do trader
		if (traderName != "")
		{
			AskalTraderConfig traderConfig = AskalTraderConfig.LoadByTraderName(traderName);
			if (traderConfig)
			{
				return traderConfig.GetOrientation();
			}
		}
		
		// Fallback: orientação padrão (0, 0, 0)
		return Vector(0.0, 0.0, 0.0);
	}
	
	// Configurar veículo após spawn (fluidos, chaves, etc)
	static void ConfigureVehicle(EntityAI vehicle, string vehicleClass)
	{
		if (!vehicle)
			return;
		
		// Configurar CarScript
		CarScript car = CarScript.Cast(vehicle);
		if (car)
		{
			// Preencher fluidos
			car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
			car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
			car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
			car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
			
			// Preencher fluidos USER se existirem
			if (car.GetFluidCapacity(CarFluid.USER1) > 0)
				car.Fill(CarFluid.USER1, car.GetFluidCapacity(CarFluid.USER1));
			if (car.GetFluidCapacity(CarFluid.USER2) > 0)
				car.Fill(CarFluid.USER2, car.GetFluidCapacity(CarFluid.USER2));
			if (car.GetFluidCapacity(CarFluid.USER3) > 0)
				car.Fill(CarFluid.USER3, car.GetFluidCapacity(CarFluid.USER3));
			if (car.GetFluidCapacity(CarFluid.USER4) > 0)
				car.Fill(CarFluid.USER4, car.GetFluidCapacity(CarFluid.USER4));
			
			car.SetSynchDirty();
			Print("[AskalVehicleSpawn] ✅ CarScript configurado: fluidos preenchidos");
		}
		
		// Configurar BoatScript
		BoatScript boat = BoatScript.Cast(vehicle);
		if (boat)
		{
			// Preencher combustível
			boat.Fill(BoatFluid.FUEL, boat.GetFluidCapacity(BoatFluid.FUEL));
			boat.SetSynchDirty();
			Print("[AskalVehicleSpawn] ✅ BoatScript configurado: combustível preenchido");
		}
		
		// TODO: Configurar chaves do veículo se necessário
		// Por enquanto, veículos spawnados ficam destravados
	}
	
	// Procurar área livre para spawn em um raio do player
	// searchRadius: raio de busca (50 metros) - onde procurar pontos candidatos
	// clearRadius: raio de área livre necessária (3 metros) - área que deve estar livre de objetos estáticos
	// Retorna primeira posição livre encontrada ou vector.Zero se não encontrar
	static vector FindFreeSpawnArea(PlayerBase player, string vehicleClass, float searchRadius, float clearRadius)
	{
		if (!player)
			return vector.Zero;
		
		Print("[AskalVehicleSpawn] 🔍 Iniciando busca de área livre:");
		Print("[AskalVehicleSpawn]   - Raio de busca: " + searchRadius.ToString() + "m");
		Print("[AskalVehicleSpawn]   - Raio de área livre necessária: " + clearRadius.ToString() + "m");
		
		vector playerPos = player.GetPosition();
		vector playerDir = player.GetDirection();
		
		// Normalizar direção
		float dirLength = playerDir.Length();
		if (dirLength < 0.001)
			playerDir = Vector(0.0, 0.0, 1.0); // Direção padrão (norte)
		else
		{
			float playerDirNormX = playerDir[0] / dirLength;
			float playerDirNormY = playerDir[1] / dirLength;
			float playerDirNormZ = playerDir[2] / dirLength;
			playerDir = Vector(playerDirNormX, playerDirNormY, playerDirNormZ);
		}
		
		// Usar abordagem de grade: verificar pontos em uma grade ao redor do player
		// Começar próximo e expandir gradualmente
		float startDistance = 5.0;
		float maxDistance = searchRadius;
		float gridStep = 3.0; // Espaçamento entre pontos na grade
		
		// Calcular direções base (forward e right) a partir da direção do player
		// Forward: direção do player
		float forwardX = playerDir[0];
		float forwardZ = playerDir[2];
		vector forward = Vector(forwardX, 0.0, forwardZ);
		// Right: perpendicular à direção (90 graus à direita)
		float rightX = -playerDir[2];
		float rightZ = playerDir[0];
		vector right = Vector(rightX, 0.0, rightZ);
		
		// Normalizar forward e right
		float forwardLen = forward.Length();
		float rightLen = right.Length();
		if (forwardLen > 0.001)
		{
			float forwardNormX = forward[0] / forwardLen;
			float forwardNormZ = forward[2] / forwardLen;
			forward = Vector(forwardNormX, 0.0, forwardNormZ);
		}
		if (rightLen > 0.001)
		{
			float rightNormX = right[0] / rightLen;
			float rightNormZ = right[2] / rightLen;
			right = Vector(rightNormX, 0.0, rightNormZ);
		}
		
		// Tentar diferentes distâncias (anéis concêntricos)
		for (float distance = startDistance; distance <= maxDistance; distance = distance + gridStep)
		{
			// Tentar pontos em uma grade ao redor do player
			// Grid de -distance até +distance em ambas direções
			int gridSize = Math.Round((distance * 2.0) / gridStep) + 1;
			
			for (int i = -gridSize; i <= gridSize; i++)
			{
				for (int j = -gridSize; j <= gridSize; j++)
				{
					// Calcular offset na grade
					float offsetForward = i * gridStep;
					float offsetRight = j * gridStep;
					
					// Calcular distância ao quadrado do centro (evita Math.Sqrt)
					float distSquared = (offsetForward * offsetForward) + (offsetRight * offsetRight);
					float startDistSquared = startDistance * startDistance;
					float maxDistSquared = distance * distance;
					
					// Pular se estiver muito longe ou muito perto
					if (distSquared < startDistSquared || distSquared > maxDistSquared)
						continue;
					
					// Calcular distância real para log (aproximação)
					float distFromCenter = distance; // Usar distância atual como aproximação
					
					// Calcular componentes do offset antes de criar Vector
					float offsetX = forward[0] * offsetForward + right[0] * offsetRight;
					float offsetY = 0.0;
					float offsetZ = forward[2] * offsetForward + right[2] * offsetRight;
					
					// Calcular posição candidata
					vector offset = Vector(offsetX, offsetY, offsetZ);
					
					// Calcular componentes da posição candidata antes de criar Vector
					float candidateX = playerPos[0] + offset[0];
					float candidateY = playerPos[1]; // Manter altura do player
					float candidateZ = playerPos[2] + offset[2];
					
					vector candidatePos = Vector(candidateX, candidateY, candidateZ);
					
					// Verificar se posição está livre (verifica área de 3m sem objetos estáticos)
					if (IsSpawnPositionFree(candidatePos, vehicleClass, clearRadius))
					{
						float actualDistance = vector.Distance(playerPos, candidatePos);
						Print("[AskalVehicleSpawn] ✅ Área livre encontrada a " + actualDistance.ToString() + "m do player");
						Print("[AskalVehicleSpawn]   Posição: " + candidatePos.ToString());
						return candidatePos;
					}
				}
			}
		}
		
		// Nenhuma área livre encontrada dentro do raio de busca
		Print("[AskalVehicleSpawn] ❌ Nenhuma área livre encontrada dentro de " + searchRadius.ToString() + "m");
		Print("[AskalVehicleSpawn]   Todas as posições verificadas tinham objetos estáticos no raio de " + clearRadius.ToString() + "m");
		return vector.Zero;
	}
	
	// Verificar se posição está livre para spawn (evitar colisões)
	// Verifica objetos estáticos e colidíveis em um raio de 3 metros
	// Retorna true se a área está livre, false se há objetos colidíveis
	static bool IsSpawnPositionFree(vector position, string vehicleClass, float radius)
	{
		if (position == vector.Zero)
			return false;
		
		// Raio de verificação: 3 metros (área livre necessária)
		float checkRadius = radius;
		
		// Buscar todos os objetos dentro do raio de verificação
		array<Object> nearbyObjects = new array<Object>();
		GetGame().GetObjectsAtPosition(position, checkRadius, nearbyObjects, NULL);
		
		Print("[AskalVehicleSpawn] 🔍 Verificando área livre em raio de " + checkRadius.ToString() + "m. Objetos encontrados: " + nearbyObjects.Count().ToString());
		
		// Verificar cada objeto encontrado
		for (int i = 0; i < nearbyObjects.Count(); i++)
		{
			Object obj = nearbyObjects.Get(i);
			if (!obj)
				continue;
			
			// Verificar se é um objeto estático ou colidível
			if (IsCollidableObject(obj))
			{
				string objType = obj.GetType();
				vector objPos = obj.GetPosition();
				float distance = vector.Distance(position, objPos);
				Print("[AskalVehicleSpawn] ⚠️ Objeto colidível encontrado: " + objType + " (distância: " + distance.ToString() + "m)");
				return false;
			}
		}
		
		// Área está livre
		Print("[AskalVehicleSpawn] ✅ Área livre confirmada (raio: " + checkRadius.ToString() + "m)");
		return true;
	}
	
	// Verificar se um objeto é colidível (estático ou veículo)
	// Verifica especificamente: Plant, HouseNoDestruct, BasebuildingBase, House, veículos, etc.
	static bool IsCollidableObject(Object obj)
	{
		if (!obj)
			return false;
		
		EntityAI entity = EntityAI.Cast(obj);
		if (!entity)
			return false;
		
		string objType = obj.GetType();
		
		// Verificar veículos (CarScript, BoatScript)
		CarScript car = CarScript.Cast(entity);
		BoatScript boat = BoatScript.Cast(entity);
		if (car || boat)
		{
			return true;
		}
		
		// Verificar objetos estáticos comuns do DayZ
		// Usar verificação de tipo por nome de classe (mais confiável)
		
		// Plant (plantas/vegetação)
		if (objType.IndexOf("Plant") >= 0)
		{
			return true;
		}
		
		// HouseNoDestruct (casas indestrutíveis)
		if (objType.IndexOf("HouseNoDestruct") >= 0)
		{
			return true;
		}
		
		// BasebuildingBase (construções de base)
		if (objType.IndexOf("BasebuildingBase") >= 0)
		{
			return true;
		}
		
		// House (casas em geral)
		if (objType.IndexOf("House") >= 0)
		{
			return true;
		}
		
		// BuildingBase (edifícios em geral)
		BuildingBase building = BuildingBase.Cast(entity);
		if (building)
		{
			return true;
		}
		
		// Fence (cercas)
		if (objType.IndexOf("Fence") >= 0)
		{
			return true;
		}
		
		// Wall (paredes)
		if (objType.IndexOf("Wall") >= 0)
		{
			return true;
		}
		
		// Gate (portões)
		if (objType.IndexOf("Gate") >= 0)
		{
			return true;
		}
		
		// Verificar se é um objeto grande (raio de bounding box > 1 metro)
		// Isso captura outros objetos estáticos que possam bloquear o spawn
		vector objPos = entity.GetPosition();
		if (objPos == vector.Zero)
			return false;
		
		// Verificar se o objeto tem um bounding box significativo
		// Usar GetBoundingBox se disponível, senão usar heurística baseada em tipo
		// Por enquanto, considerar apenas os tipos conhecidos acima
		
		// Se chegou aqui, não é um objeto colidível conhecido
		return false;
	}
}

