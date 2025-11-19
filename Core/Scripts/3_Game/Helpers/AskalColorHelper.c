// ===========================================================================
// Askal Color Helper
// Utility functions for color manipulation and conversion
// ===========================================================================

class AskalColorHelper
{
	// Converts HSV color space to RGB color space
	// H: Hue (0-360), S: Saturation (0-1), V: Value (0-1)
	// Returns: r, g, b (0-255)
	static void HSVtoRGB(float h, float s, float v, out int r, out int g, out int b)
	{
		float c = v * s;
		float h_sector = h / 60.0;
		float x = c * (1.0 - Math.AbsFloat(h_sector - 2.0 * Math.Floor(h_sector / 2.0) - 1.0));
		float m = v - c;
		
		float r_prime, g_prime, b_prime;
		
		if (h >= 0 && h < 60)
		{
			r_prime = c;
			g_prime = x;
			b_prime = 0;
		}
		else if (h >= 60 && h < 120)
		{
			r_prime = x;
			g_prime = c;
			b_prime = 0;
		}
		else if (h >= 120 && h < 180)
		{
			r_prime = 0;
			g_prime = c;
			b_prime = x;
		}
		else if (h >= 180 && h < 240)
		{
			r_prime = 0;
			g_prime = x;
			b_prime = c;
		}
		else if (h >= 240 && h < 300)
		{
			r_prime = x;
			g_prime = 0;
			b_prime = c;
		}
		else
		{
			r_prime = c;
			g_prime = 0;
			b_prime = x;
		}
		
		r = (int)((r_prime + m) * 255.0);
		g = (int)((g_prime + m) * 255.0);
		b = (int)((b_prime + m) * 255.0);
	}
	
	// Gets color based on health percentage (0-100)
	// Returns ARGB integer for use with Widget.SetColor()
	// 100% = Green (120° hue), 0% = Red (0° hue)
	static int GetHealthColor(float healthPercent)
	{
		if (healthPercent < 0)
			healthPercent = 0;
		if (healthPercent > 100)
			healthPercent = 100;
		
		// Map health to hue: 0% = Red (0°), 100% = Green (120°)
		float hue = (healthPercent / 100.0) * 120.0;
		
		int r, g, b;
		HSVtoRGB(hue, 1.0, 1.0, r, g, b);
		
		// Return ARGB format (full alpha = 255)
		return ARGB(255, r, g, b);
	}
	
	// Converts health percentage to color and applies to widget
	static void ApplyHealthColor(Widget widget, float healthPercent)
	{
		if (!widget)
			return;
		
		int color = GetHealthColor(healthPercent);
		widget.SetColor(color);
	}
}

