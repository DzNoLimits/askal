// ==========================================================
//	Askal Market Constants
//	Currency mode constants and default values
// ==========================================================

class AskalMarketConstants
{
	// Currency Mode Constants
	const int CURRENCY_MODE_DISABLED = 0;    // Currency is disabled, reject all transactions
	const int CURRENCY_MODE_PHYSICAL = 1;    // Physical currency with item classes (default)
	const int CURRENCY_MODE_VIRTUAL = 2;     // Virtual currency, no physical items
	
	// Default currency ID fallback
	const string DEFAULT_CURRENCY_ID = "Askal_Money";
}

