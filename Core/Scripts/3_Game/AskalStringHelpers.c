// ==========================================
// AskalStringHelpers - Funções auxiliares para manipulação de strings
// ==========================================

class AskalStringHelpers
{
	// Dividir string por delimitador (ex: "item1,item2,item3" -> ["item1", "item2", "item3"])
	// Mais eficiente e legível que parse manual com IndexOf/Substring
	static array<string> SplitString(string input, string delimiter)
	{
		array<string> result = new array<string>();
		
		if (!input || input == "" || !delimiter || delimiter == "")
		{
			if (input && input != "")
				result.Insert(input);
			return result;
		}
		
		int startPos = 0;
		int delimiterLength = delimiter.Length();
		
		while (startPos < input.Length())
		{
			int delimiterPos = input.IndexOfFrom(startPos, delimiter);
			string segment = "";
			
			if (delimiterPos == -1)
			{
				// Último segmento (ou string inteira se não há delimitador)
				segment = input.Substring(startPos, input.Length() - startPos);
				if (segment != "")
					result.Insert(segment);
				break;
			}
			else
			{
				// Segmento antes do delimitador
				segment = input.Substring(startPos, delimiterPos - startPos);
				if (segment != "")
					result.Insert(segment);
				
				// Avançar após o delimitador
				startPos = delimiterPos + delimiterLength;
			}
		}
		
		return result;
	}
	
	// Dividir string por vírgula (caso comum)
	static array<string> SplitByComma(string input)
	{
		return SplitString(input, ",");
	}
	
	// Remover espaços em branco do início e fim da string
	static string Trim(string input)
	{
		if (!input || input == "")
			return "";
		
		int start = 0;
		int end = input.Length() - 1;
		
		// Encontrar início (pular espaços)
		while (start <= end && input.Substring(start, 1) == " ")
			start++;
		
		// Encontrar fim (pular espaços)
		while (end >= start && input.Substring(end, 1) == " ")
			end--;
		
		if (start > end)
			return "";
		
		return input.Substring(start, end - start + 1);
	}
	
	// Dividir string por vírgula e remover espaços (caso comum para arrays)
	static array<string> SplitByCommaTrimmed(string input)
	{
		array<string> parts = SplitByComma(input);
		array<string> trimmed = new array<string>();
		
		for (int i = 0; i < parts.Count(); i++)
		{
			string trimmedPart = Trim(parts.Get(i));
			if (trimmedPart != "")
				trimmed.Insert(trimmedPart);
		}
		
		return trimmed;
	}
}

