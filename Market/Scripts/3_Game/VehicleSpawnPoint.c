// ==========================================
// VehicleSpawnPoint - Ponto de spawn de veículo
// ==========================================

class VehicleSpawnPoint
{
	ref array<float> Position; // [x, y, z]
	ref array<float> Rotation; // [yaw, pitch, roll]
	
	void VehicleSpawnPoint()
	{
		Position = new array<float>();
		Rotation = new array<float>();
	}
	
	// Obter posição como vector
	vector GetPosition()
	{
		if (!Position)
		{
			Print("[VehicleSpawnPoint] ⚠️ Position é NULL");
			return vector.Zero;
		}
		
		int count = Position.Count();
		Print("[VehicleSpawnPoint] 📍 Position array tem " + count.ToString() + " elementos");
		
		if (count != 3)
		{
			Print("[VehicleSpawnPoint] ⚠️ Position inválido (Count: " + count.ToString() + ", esperado: 3)");
			return vector.Zero;
		}
		
		// DayZ Vector usa ordem [X, Y, Z] onde:
		// X = Leste/Oeste (horizontal)
		// Y = Altura (vertical) 
		// Z = Norte/Sul (profundidade)
		float posX = Position.Get(0);
		float posY = Position.Get(1);
		float posZ = Position.Get(2);
		
		Print("[VehicleSpawnPoint] 📍 Lendo posição do JSON:");
		Print("[VehicleSpawnPoint]   Índice 0 (X): " + posX.ToString());
		Print("[VehicleSpawnPoint]   Índice 1 (Y): " + posY.ToString());
		Print("[VehicleSpawnPoint]   Índice 2 (Z): " + posZ.ToString());
		
		// Verificar se valores são válidos
		if (posX == 0.0 && posY == 0.0 && posZ == 0.0)
		{
			Print("[VehicleSpawnPoint] ⚠️ Posição é zero (0, 0, 0)");
		}
		
		// Criar Vector na ordem correta [X, Y, Z]
		vector result = Vector(posX, posY, posZ);
		Print("[VehicleSpawnPoint] 📍 Vector criado: " + result.ToString());
		Print("[VehicleSpawnPoint] 📍 Vector componentes: X=" + result[0].ToString() + " Y=" + result[1].ToString() + " Z=" + result[2].ToString());
		
		return result;
	}
	
	// Obter rotação como vector
	vector GetRotation()
	{
		if (!Rotation || Rotation.Count() != 3)
			return vector.Zero;
		
		return Vector(Rotation.Get(0), Rotation.Get(1), Rotation.Get(2));
	}
}

