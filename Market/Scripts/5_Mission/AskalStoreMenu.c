// ========================================
// ENUMS - DEFINIDOS FORA DA CLASSE
// ========================================

/// Enum para tipos de quantidade de itens
enum AskalItemQuantityType
{
	NONE = 0,        // Item normal (sem quantidade variável)
	MAGAZINE = 1,    // Carregador de arma (munição)
	STACKABLE = 2,   // Itens empilháveis (pregos, balas, etc)
	QUANTIFIABLE = 3 // Itens com quantidade fracionária (bandagens, meat, water, etc)
}

// Informações auxiliares para cards de inventário no modo de venda em lote
class AskalInventoryDisplayInfo
{
	EntityAI Item;
	string ClassName;
	string DisplayName;
	int EstimatedPrice;
	float HealthPercent;
}

	// ========================================
// CLASSE PRINCIPAL DO MENU
	// ========================================
	
class AskalStoreMenu extends UIScriptedMenu
{
	protected static const string DEFAULT_DATASET_ICON = "set:dayz_inventory image:missing";
	protected static const int DEFAULT_HARDCODED_BUY_PRICE = 123;
	protected static const int DEFAULT_HARDCODED_SELL_PERCENT = 50;
	protected static AskalStoreMenu s_Instance;
	
	static AskalStoreMenu GetInstance()
	{
		return s_Instance;
	}
	// ========================================
	// WIDGETS - ESTRUTURA COMPLETA
	// ========================================
	protected Widget m_RootWidget;
	protected ButtonWidget m_CloseButton;
	
	// Painel Central - Lista de Itens
	protected ScrollWidget m_ItensCardScroll;
	protected WrapSpacerWidget m_ItensCardWrap;
	protected ref array<Widget> m_ItemWidgets;
	protected ref map<Widget, int> m_ItemCardToIndex;
	
	// Painel Direito - Info Detalhada (item_details.layout)
	protected Widget m_ItemDetailsHolder;
	protected ItemPreviewWidget m_SelectedItemPreview;
	protected TextWidget m_SelectedItemName;
	
	// Sistema de Variantes
	protected ScrollWidget m_VariantsScroll;
	protected Widget m_VariantsCardHolder; // GridSpacerWidget
	protected ref array<Widget> m_VariantWidgets;
	protected ref map<Widget, string> m_VariantCardToClassName;
	
	// Sistema de Attachments
	protected WrapSpacerWidget m_AttachmentsCardHolder;
	protected ref array<Widget> m_AttachmentWidgets;
	protected ref map<Widget, string> m_AttachmentCardToSlot;
	
	// Category Cards (header)
	protected WrapSpacerWidget m_CategoryCardHolder;
	protected ref array<Widget> m_CategoryCardRoots;
	protected ref array<TextWidget> m_CategorySelectedTexts;
	protected ref array<TextWidget> m_CategoryUnselectedTexts;
	protected ref array<ButtonWidget> m_CategorySelectedButtons;
	protected ref array<ButtonWidget> m_CategoryUnselectedButtons;
	protected ref array<string> m_ItemDatasetIds;
	protected ref array<string> m_ItemCategoryIds;
	
	// Dataset Cards (painel esquerdo)
	protected ScrollWidget m_DatasetCardsScroll;
	protected GridSpacerWidget m_DatasetCardsHolder;
	protected ref array<ButtonWidget> m_DatasetButtons;
	protected ref array<Widget> m_DatasetCardRoots;
	protected ref array<ImageWidget> m_DatasetIconWidgets;
	protected ref array<string> m_DatasetDisplayNames;
	protected ref array<bool> m_DatasetExpandedStates;
	protected ref array<bool> m_DatasetCategoriesBuilt;
	protected ref array<GridSpacerWidget> m_DatasetCategoryHolders;
	protected ref array<ref array<Widget>> m_DatasetCategoryCardRootsPerDataset;
	protected ref array<ref array<ButtonWidget>> m_DatasetCategoryButtonsPerDataset;
	protected ref array<ref array<TextWidget>> m_DatasetCategoryTextsPerDataset;
	protected ref array<ref array<string>> m_DatasetCategoryIDsPerDataset;
	protected ref array<ref array<string>> m_DatasetCategoryDisplayNamesPerDataset;
	
	// Breadcrumb / Transaction
	protected TextWidget m_ItemsBreadcrumbText;
	protected TextWidget m_TransactionDescription;
	protected EditBoxWidget m_TransactionQuantityInput;
	protected ButtonWidget m_TransactionQuantityPlusButton;
	protected ButtonWidget m_TransactionQuantityMinusButton;
	protected int m_TransactionQuantity = 1;
	protected int m_SelectedItemUnitPrice = 0;
	
	// Slider de Quantidade (para magazines, stackables, etc)
	protected SliderWidget m_TransactionQuantitySlider;
	protected TextWidget m_TransactionQuantitySliderText; // Texto central do slider
	protected int m_CurrentQuantityPercent = 100; // Percentual de quantidade (0-100)
	protected int m_CurrentAmmoCount = 0; // Para magazines
	protected int m_SliderMinValue = 0;
	protected int m_SliderMaxValue = 100;
	protected float m_SliderStepValue = 1.0;
	protected AskalItemQuantityType m_SliderQuantityType = AskalItemQuantityType.NONE;
	protected string m_SliderUnitLabel = "%";
	protected string m_SliderCurrentClassName = "";
	
	// Seletor de Conteúdo (para magazines e Bottle_Base)
	protected Widget m_TransactionContentPanel;
	protected TextWidget m_TransactionContentLabel;
	protected ButtonWidget m_TransactionContentPrevButton;
	protected ButtonWidget m_TransactionContentNextButton;
	protected ref array<string> m_AvailableContentTypes; // Munições compatíveis ou identificadores de líquido (string)
	protected ref array<int> m_AvailableLiquidTypes; // liquidType integers para Bottle_Base
	protected ref array<float> m_AvailableContentUnitPrices; // Preço por unidade (bala ou mL) para o conteúdo
	protected int m_CurrentContentIndex = 0; // Índice do conteúdo selecionado
	protected string m_CurrentSelectedContent = ""; // Nome do conteúdo atual (ex: "Ammo_308Win" ou liquidType)
	protected bool m_CurrentItemIsLiquidContainer = false;
	protected float m_CurrentLiquidCapacity = 0.0; // Capacidade total (mL) do recipiente atual
	
	// Search / Filters
	protected EditBoxWidget m_SearchInput;
	protected ButtonWidget m_SearchButton;
	
	// Hover Text
	protected Widget m_HoverPanel;
	protected TextWidget m_HoverText;
	
	// Botões de Ação
	protected Widget m_BuySellDualPanel;
	protected ButtonWidget m_BuyButton;
	protected ButtonWidget m_SellButton;
	protected Widget m_BuySellSinglePanel;
	protected ButtonWidget m_BuyButtonSolo;
	protected ButtonWidget m_SellButtonSolo;
	protected MultilineTextWidget m_BuyTotalTextDual;
	protected MultilineTextWidget m_SellTotalTextDual;
	protected MultilineTextWidget m_BuyTotalTextWide;
	protected MultilineTextWidget m_SellTotalTextWide;
	protected TextWidget m_BuyCurrencyShortDual;
	protected TextWidget m_SellCurrencyShortDual;
	protected TextWidget m_BuyCurrencyShortWide;
	protected TextWidget m_SellCurrencyShortWide;
	
	// Toggles de compra/venda em lote
	protected ButtonWidget m_BatchBuyToggle;
	protected ButtonWidget m_BatchSellToggle;
	protected bool m_BatchBuyEnabled = false;
	protected bool m_BatchSellEnabled = false;
	protected ref map<Widget, bool> m_ItemCardSelectionState;
	protected ref map<int, bool> m_BatchBuySelectedIndexes;
	protected ref array<EntityAI> m_BatchSellSelectedEntities;
	
	// UI especial para inventário do player
	protected Widget m_PlayerInventoryCard;
	protected ButtonWidget m_PlayerInventoryHeaderButton;
	protected GridSpacerWidget m_PlayerInventoryCategoryHolder;
	protected bool m_ShowingInventoryForSale = false;

	// Sistema de cooldown
	protected ProgressBarWidget m_BuyCooldownProgressBar;
	protected ProgressBarWidget m_SellCooldownProgressBar;
	protected ProgressBarWidget m_BuySoloCooldownProgressBar;
	protected ProgressBarWidget m_SellSoloCooldownProgressBar;
	protected float m_CooldownDuration = 0.5; // segundos
	protected ref map<ButtonWidget, float> m_ButtonCooldownStartTimes;
	
	// Sistema de Inventário (para venda)
	protected ref map<string, ref array<EntityAI>> m_PlayerInventoryItems; // className -> lista de EntityAI
	protected ref map<string, float> m_ItemHealthMap; // className -> healthPercent (do servidor)
	protected ref map<Widget, EntityAI> m_ItemCardToInventoryItem; // Card -> item real do inventário
	protected EntityAI m_SelectedInventoryItem;
	protected bool m_InventoryScanned = false;
	protected ref array<ref AskalInventoryDisplayInfo> m_InventoryDisplayItems;
	
	// Sistema de Notificações
	protected WrapSpacerWidget m_NotificationCardHolder;
	protected ref array<Widget> m_NotificationCards;
	protected ref array<float> m_NotificationTimestamps;
	protected ref array<Widget> m_NotificationSlidePanels;
	protected ref array<float> m_NotificationAnimationStartTimes;
	protected ref array<float> m_NotificationBasePosY;
	protected ref array<float> m_NotificationSlideParentWidths;
	protected const float NOTIFICATION_LIFETIME = 5.0; // 5 segundos
	protected const int MAX_NOTIFICATIONS = 10; // Máximo de notificações visíveis
	protected const float NOTIFICATION_ANIMATION_DURATION = 0.5; // Meio segundo para deslizar
	
	// Configuração do Trader Atual
	protected string m_CurrentTraderName;
	protected ref map<string, int> m_TraderSetupItems; // SetupItems do trader atual
	protected TextWidget m_HeaderTitleText; // Título do menu (market_header_title_text)
	
	// Cache de resolução de dataset/categoria (performance)
	protected ref map<string, Param2<string, string>> m_ItemToDatasetCategoryCache;
	
	// ========================================
	// DADOS
	// ========================================
	protected ref array<ref AskalItemData> m_Items;
	protected ref array<string> m_Datasets;
	protected ref array<string> m_Categories;
	protected ref array<string> m_CategoryDisplayNames;
	protected int m_CurrentDatasetIndex = 0;
	protected int m_CurrentCategoryIndex = 0;
	protected int m_SelectedItemIndex = -1;
	protected string m_SelectedVariantClass = "";
	protected string m_CurrentVariantBaseClass = "";
	protected string m_ActiveCurrencyId = "";
	protected string m_CurrentCurrencyShortName = "";
	protected string m_CurrentSelectedClassName = "";
	protected int m_CurrentItemMode = 3;
	protected bool m_CurrentCanBuy = true;
	protected bool m_CurrentCanSell = true;
	protected int m_CurrentActionLayout = 0; // 0=hidden, 1=buy wide, 2=sell wide, 3=dual
	protected bool m_VirtualStoreConfigLoaded = false;
	protected string m_VirtualStoreCurrencyId = "";
	protected ref map<string, int> m_VirtualStoreSetupModes;
	protected float m_BuyCoefficient = 1.0;
	protected float m_SellCoefficient = 1.0;
protected bool m_WaitingVirtualStoreConfig = false;
protected string m_LastVirtualStoreConfigSignature = "";
	
	
	// Preview 3D (rotação automática)
	protected ref array<EntityAI> m_PreviewItems;
	protected float m_PreviewRotation = 0.0;
	protected const float PREVIEW_ROTATION_SPEED = 20.0; // graus/segundo
	protected vector m_PreviewPosition = Vector(0, 0, 0.5);
	protected float m_PreviewZoom = 0.55;

	// ========================================
	// CONTEXTO / HELPERS
	// ========================================

	protected bool IsDedicatedServerContext()
	{
		DayZGame dzGame = DayZGame.Cast(GetGame());
		if (!dzGame)
			return false;
		return dzGame.IsDedicatedServer();
	}
	
	protected Object SpawnTemporaryObject(string className)
	{
		DayZGame dzGame = DayZGame.Cast(GetGame());
		if (!dzGame)
			return null;
		if (dzGame.IsDedicatedServer())
			return null;
		return dzGame.CreateObject(className, vector.Zero, true, false, true);
	}
	
	void AskalStoreMenu()
	{
		Print("[AskalStore] =============================================");
		Print("[AskalStore] AskalStoreMenu() CONSTRUTOR - VERSÃO DINÂMICA");
		Print("[AskalStore] =============================================");
		
		s_Instance = this;
		
		m_Items = new array<ref AskalItemData>();
		m_Datasets = new array<string>();
		m_Categories = new array<string>();
		m_CategoryDisplayNames = new array<string>();
		m_ItemWidgets = new array<Widget>();
		m_ItemCardToIndex = new map<Widget, int>();
		m_VariantWidgets = new array<Widget>();
		m_VariantCardToClassName = new map<Widget, string>();
		m_AttachmentWidgets = new array<Widget>();
		m_AttachmentCardToSlot = new map<Widget, string>();
		m_DatasetButtons = new array<ButtonWidget>();
		m_DatasetCardRoots = new array<Widget>();
		m_DatasetIconWidgets = new array<ImageWidget>();
		m_DatasetDisplayNames = new array<string>();
		m_DatasetExpandedStates = new array<bool>();
		m_DatasetCategoriesBuilt = new array<bool>();
		m_DatasetCategoryHolders = new array<GridSpacerWidget>();
		m_DatasetCategoryCardRootsPerDataset = new array<ref array<Widget>>();
		m_DatasetCategoryButtonsPerDataset = new array<ref array<ButtonWidget>>();
		m_DatasetCategoryTextsPerDataset = new array<ref array<TextWidget>>();
		m_DatasetCategoryIDsPerDataset = new array<ref array<string>>();
		m_DatasetCategoryDisplayNamesPerDataset = new array<ref array<string>>();
		m_CategoryCardRoots = new array<Widget>();
		m_CategorySelectedTexts = new array<TextWidget>();
		m_CategoryUnselectedTexts = new array<TextWidget>();
		m_CategorySelectedButtons = new array<ButtonWidget>();
		m_CategoryUnselectedButtons = new array<ButtonWidget>();
		m_ItemDatasetIds = new array<string>();
		m_ItemCategoryIds = new array<string>();
		m_DatasetCategoryHolders = new array<GridSpacerWidget>();
		m_PreviewItems = new array<EntityAI>();
		m_NotificationCards = new array<Widget>();
		m_NotificationTimestamps = new array<float>();
		m_NotificationSlidePanels = new array<Widget>();
		
		// Inicializar configuração do trader
		m_CurrentTraderName = "";
		m_TraderSetupItems = new map<string, int>();
		m_ItemToDatasetCategoryCache = new map<string, Param2<string, string>>();
		m_NotificationAnimationStartTimes = new array<float>();
		m_NotificationBasePosY = new array<float>();
		m_NotificationSlideParentWidths = new array<float>();
		m_ItemCardSelectionState = new map<Widget, bool>();
		m_BatchBuySelectedIndexes = new map<int, bool>();
		m_BatchSellSelectedEntities = new array<EntityAI>();
		m_InventoryDisplayItems = new array<ref AskalInventoryDisplayInfo>();
		m_VirtualStoreSetupModes = new map<string, int>();
		m_ButtonCooldownStartTimes = new map<ButtonWidget, float>();
		
	}
	
	protected void LoadVirtualStoreConfig()
	{
		if (m_VirtualStoreConfigLoaded)
		{
			EnsureVirtualStoreConfigApplied();
			return;
		}
		
		if (!m_VirtualStoreSetupModes)
			m_VirtualStoreSetupModes = new map<string, int>();
		else
			m_VirtualStoreSetupModes.Clear();
		
		m_VirtualStoreConfigLoaded = true;
		
		EnsureVirtualStoreConfigApplied();
		m_WaitingVirtualStoreConfig = true;
		RequestVirtualStoreConfig();
	}
	
	protected string ComputeVirtualStoreConfigSignature(AskalVirtualStoreConfig storeConfig)
	{
		if (!storeConfig)
			return "";
		
		string currencyId = storeConfig.GetPrimaryCurrency();
		string signature = currencyId;
		signature += "|" + string.Format("%1", storeConfig.BuyCoefficient);
		signature += "|" + string.Format("%1", storeConfig.SellCoefficient);
		
		if (storeConfig.SetupItems)
		{
			ref array<string> keys = new array<string>();
			for (int i = 0; i < storeConfig.SetupItems.Count(); i++)
			{
				string key = storeConfig.SetupItems.GetKey(i);
				if (key && key != "")
					keys.Insert(key);
			}
			keys.Sort();
			for (int k = 0; k < keys.Count(); k++)
			{
				string sortedKey = keys.Get(k);
				int sortedMode = storeConfig.SetupItems.Get(sortedKey);
				signature += "|" + sortedKey + ":" + sortedMode;
			}
		}
		
		return signature;
	}
	
	protected void EnsureVirtualStoreConfigApplied()
	{
		if (!m_WaitingVirtualStoreConfig && !AskalVirtualStoreSettings.IsConfigSynced())
			return;
		
		AskalVirtualStoreConfig storeConfig = AskalVirtualStoreSettings.GetConfig();
		if (!storeConfig)
			return;
		
		string signature = ComputeVirtualStoreConfigSignature(storeConfig);
		if (!m_WaitingVirtualStoreConfig && signature == m_LastVirtualStoreConfigSignature)
			return;
		
		ref array<string> setupKeys = new array<string>();
		ref array<int> setupValues = new array<int>();
		if (storeConfig.SetupItems)
		{
			for (int i = 0; i < storeConfig.SetupItems.Count(); i++)
			{
				string key = storeConfig.SetupItems.GetKey(i);
				if (key && key != "")
				{
					setupKeys.Insert(key);
					int mode = storeConfig.SetupItems.GetElement(i);
					setupValues.Insert(mode);
				}
			}
		}
		
		ApplyVirtualStoreConfig(storeConfig.GetPrimaryCurrency(), storeConfig.BuyCoefficient, storeConfig.SellCoefficient, setupKeys, setupValues);
		m_LastVirtualStoreConfigSignature = signature;
		m_WaitingVirtualStoreConfig = false;
	}
	
	protected void RequestVirtualStoreConfig()
	{
		if (GetGame() && GetGame().IsClient())
		{
			m_WaitingVirtualStoreConfig = true;
			GetRPCManager().SendRPC("AskalCoreModule", "RequestVirtualStoreConfig", NULL, true, NULL, NULL);
		}
	}
	
	protected void ApplyVirtualStoreConfig(string currencyId, float buyCoeff, float sellCoeff, array<string> setupKeys, array<int> setupValues)
	{
		if (!m_VirtualStoreSetupModes)
			m_VirtualStoreSetupModes = new map<string, int>();
		else
			m_VirtualStoreSetupModes.Clear();
		
		if (setupKeys && setupValues && setupKeys.Count() == setupValues.Count())
		{
			for (int i = 0; i < setupKeys.Count(); i++)
			{
				string key = setupKeys.Get(i);
				int mode = setupValues.Get(i);
				if (key && key != "")
					m_VirtualStoreSetupModes.Set(key, mode);
			}
		}
		
		if (buyCoeff <= 0)
			buyCoeff = 1.0;
		if (sellCoeff <= 0)
			sellCoeff = 1.0;
		m_BuyCoefficient = buyCoeff;
		m_SellCoefficient = sellCoeff;
		
		// Resolve currency for virtual store
		AskalCurrencyConfig resolvedCurrencyCfg = NULL;
		string resolvedCurrencyId = "";
		if (!currencyId || currencyId == "")
			currencyId = "";
		if (AskalMarketConfig.ResolveAcceptedCurrency("", currencyId, resolvedCurrencyId, resolvedCurrencyCfg))
		{
			m_VirtualStoreCurrencyId = resolvedCurrencyId;
			m_ActiveCurrencyId = resolvedCurrencyId;
			Print("[AskalStore] 💰 Currency resolvida para virtual store: " + resolvedCurrencyId);
		}
		else
		{
			// Fallback
			if (!currencyId || currencyId == "")
				currencyId = "Askal_Money";
			m_VirtualStoreCurrencyId = currencyId;
			m_ActiveCurrencyId = currencyId;
			Print("[AskalStore] ⚠️ Usando currency padrão: " + currencyId);
		}
		
		RefreshCurrencyShortname();
		UpdateTransactionSummary();
	}
	
	override Widget Init()
	{
		Print("[AskalStore] ========================================");
		Print("[AskalStore] Init() - VERSÃO DINÂMICA (lê do Core Database)");
		Print("[AskalStore] ========================================");
		
		// Carregar layout principal (NOVO LAYOUT)
		m_RootWidget = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_window.layout");
		if (!m_RootWidget)
		{
			Print("[AskalStore] ❌ ERRO: RootWidget é NULL!");
			return NULL;
		}
		
		Print("[AskalStore] ✅ RootWidget criado!");
		
		// Widget do título do menu
		m_HeaderTitleText = TextWidget.Cast(m_RootWidget.FindAnyWidget("market_header_title_text"));
		if (m_HeaderTitleText)
		{
			Print("[AskalStore] ✅ market_header_title_text encontrado!");
		}
		else
		{
			Print("[AskalStore] ⚠️ market_header_title_text NÃO encontrado!");
		}
		
		// Widgets básicos (PADRONIZADOS)
		m_CloseButton = ButtonWidget.Cast(m_RootWidget.FindAnyWidget("market_close_button"));
		m_ItensCardScroll = ScrollWidget.Cast(m_RootWidget.FindAnyWidget("items_list_scroll_container"));
		m_ItensCardWrap = WrapSpacerWidget.Cast(m_RootWidget.FindAnyWidget("items_cards_holder"));
		
		// Dataset Cards Holder - CORRIGIDO: nome correto é datasets_scroll_container
		Print("[AskalStore] 🔍 Buscando datasets_scroll_container...");
		m_DatasetCardsScroll = ScrollWidget.Cast(m_RootWidget.FindAnyWidget("datasets_scroll_container"));
		if (m_DatasetCardsScroll)
		{
			Print("[AskalStore] ✅ datasets_scroll_container encontrado!");
			// Cada dataset tem seu próprio category_cards_holder dentro de dataset_card.layout
            m_DatasetCardsHolder = GridSpacerWidget.Cast(m_DatasetCardsScroll.FindAnyWidget("datasets_cards_holder"));
			if (m_DatasetCardsHolder)
				Print("[AskalStore] ✅ datasets_cards_holder encontrado!");
			else
				Print("[AskalStore] ❌ datasets_cards_holder NÃO encontrado!");
		}
		else
		{
			Print("[AskalStore] ❌ datasets_scroll_container NÃO encontrado!");
		}
		
		// Item Details Holder (item_details_panel já está no layout)
		m_ItemDetailsHolder = m_RootWidget.FindAnyWidget("item_details_panel");
		if (m_ItemDetailsHolder)
		{
			// No novo layout, os widgets estão em item_details_panel > item_details_container
			Widget itemDetailsRoot = m_ItemDetailsHolder.FindAnyWidget("item_details_container");
			if (itemDetailsRoot)
			{
				Print("[AskalStore] ✅ Item details layout carregado!");
				// Widgets do item_details
				m_SelectedItemPreview = ItemPreviewWidget.Cast(itemDetailsRoot.FindAnyWidget("item_preview_widget"));
				m_SelectedItemName = TextWidget.Cast(m_RootWidget.FindAnyWidget("details_section_title_text"));
				if (!m_SelectedItemName)
					m_SelectedItemName = TextWidget.Cast(itemDetailsRoot.FindAnyWidget("details_section_title_text"));
				if (m_SelectedItemName)
					Print("[AskalStore] ✅ details_section_title_text encontrado!");
				else
					Print("[AskalStore] ⚠️ details_section_title_text NÃO encontrado!");
				m_VariantsScroll = ScrollWidget.Cast(itemDetailsRoot.FindAnyWidget("variants_scroll_container"));
				m_VariantsCardHolder = itemDetailsRoot.FindAnyWidget("variants_cards_holder");
				m_AttachmentsCardHolder = WrapSpacerWidget.Cast(itemDetailsRoot.FindAnyWidget("attachments_cards_holder"));
			}
			else
			{
				Print("[AskalStore] ❌ ERRO: itemDetailsRoot é NULL!");
			}
		}
		
		// Botões de Ação - CORRIGIDO: actions_panel está em m_RootWidget, NÃO em itemDetailsRoot!
		Print("[AskalStore] 🔍 Buscando actions_panel no m_RootWidget...");
		Widget actionsPanel = m_RootWidget.FindAnyWidget("actions_panel");
		if (actionsPanel)
		{
			Print("[AskalStore] ✅ actions_panel encontrado!");
			
			// Painel Dual (botões lado a lado)
			m_BuySellDualPanel = actionsPanel.FindAnyWidget("buy_sell_dual_panel");
			if (!m_BuySellDualPanel)
			{
				m_BuySellDualPanel = actionsPanel.FindAnyWidget("buy_and_sell_button");
				if (m_BuySellDualPanel)
					Print("[AskalStore] ✅ buy_and_sell_button encontrado (novo layout)!");
			}
			
			if (m_BuySellDualPanel)
			{
				Print("[AskalStore] ✅ Painel dual de compra/venda identificado");
				m_BuyButton = ButtonWidget.Cast(m_BuySellDualPanel.FindAnyWidget("buy_button"));
				if (!m_BuyButton)
					m_BuyButton = ButtonWidget.Cast(actionsPanel.FindAnyWidget("buy_button"));
				
				m_SellButton = ButtonWidget.Cast(m_BuySellDualPanel.FindAnyWidget("sell_button"));
				if (!m_SellButton)
					m_SellButton = ButtonWidget.Cast(actionsPanel.FindAnyWidget("sell_button"));
			}
			else
			{
				Print("[AskalStore] ❌ Painel dual não encontrado, procurando botões diretamente no actions_panel");
				m_BuyButton = ButtonWidget.Cast(actionsPanel.FindAnyWidget("buy_button"));
				m_SellButton = ButtonWidget.Cast(actionsPanel.FindAnyWidget("sell_button"));
			}
			
			if (m_BuyButton)
			{
				Print("[AskalStore] ✅ buy_button encontrado!");
				m_BuyTotalTextDual = MultilineTextWidget.Cast(m_BuyButton.FindAnyWidget("buy_button_total_text"));
				m_BuyCurrencyShortDual = TextWidget.Cast(m_BuyButton.FindAnyWidget("currency_shortname_0"));
			}
			else
			{
				Print("[AskalStore] ❌ buy_button NÃO encontrado!");
			}
			
			if (m_SellButton)
			{
				Print("[AskalStore] ✅ sell_button encontrado!");
				m_SellTotalTextDual = MultilineTextWidget.Cast(m_SellButton.FindAnyWidget("sell_button_total_text"));
				m_SellCurrencyShortDual = TextWidget.Cast(m_SellButton.FindAnyWidget("currency_shortname_1"));
			}
			else
			{
				Print("[AskalStore] ❌ sell_button NÃO encontrado!");
			}
			
			// Painel Single (botões grandes individuais)
			m_BuySellSinglePanel = actionsPanel.FindAnyWidget("buy_sell_single_panel");
			if (!m_BuySellSinglePanel)
			{
				m_BuySellSinglePanel = actionsPanel.FindAnyWidget("buy_sell_single_container");
			}
			
			if (m_BuySellSinglePanel)
				Print("[AskalStore] ✅ Contêiner single de compra/venda identificado!");
			else
				Print("[AskalStore] ⚠️ Contêiner single não identificado no layout (usando botões diretos)");
			
			if (m_BuySellSinglePanel)
			{
				m_BuyButtonSolo = ButtonWidget.Cast(m_BuySellSinglePanel.FindAnyWidget("buy_button_solo"));
				m_SellButtonSolo = ButtonWidget.Cast(m_BuySellSinglePanel.FindAnyWidget("sell_button_solo"));
			}
			
			if (!m_BuyButtonSolo)
				m_BuyButtonSolo = ButtonWidget.Cast(actionsPanel.FindAnyWidget("buy_button_solo"));
			if (!m_BuyButtonSolo)
				m_BuyButtonSolo = ButtonWidget.Cast(actionsPanel.FindAnyWidget("buy_only_button_wide"));
			
			if (!m_SellButtonSolo)
				m_SellButtonSolo = ButtonWidget.Cast(actionsPanel.FindAnyWidget("sell_button_solo"));
			if (!m_SellButtonSolo)
				m_SellButtonSolo = ButtonWidget.Cast(actionsPanel.FindAnyWidget("sell_only_button_wide"));
			
			if (m_BuyButtonSolo)
			{
				Print("[AskalStore] ✅ Botão individual de compra localizado!");
				m_BuyTotalTextWide = MultilineTextWidget.Cast(m_BuyButtonSolo.FindAnyWidget("buy_button_total_text_wide"));
				m_BuyCurrencyShortWide = TextWidget.Cast(m_BuyButtonSolo.FindAnyWidget("currency_shortname_2"));
		}
		else
		{
				Print("[AskalStore] ❌ Botão individual de compra NÃO encontrado!");
			}
			
			if (m_SellButtonSolo)
			{
				Print("[AskalStore] ✅ Botão individual de venda localizado!");
				m_SellTotalTextWide = MultilineTextWidget.Cast(m_SellButtonSolo.FindAnyWidget("sell_button_total_text_wide"));
				m_SellCurrencyShortWide = TextWidget.Cast(m_SellButtonSolo.FindAnyWidget("currency_shortname_3"));
			}
			else
			{
				Print("[AskalStore] ❌ Botão individual de venda NÃO encontrado!");
			}
			
			RefreshCurrencyShortname();
			ResetButtonTotals();
		}
		else
		{
			Print("[AskalStore] ❌ actions_panel NÃO encontrado no m_RootWidget!");
		}
		
		// Busca rápida (categories_section_header > search_bar)
		m_SearchInput = EditBoxWidget.Cast(m_RootWidget.FindAnyWidget("search_input"));
		if (m_SearchInput)
			Print("[AskalStore] ✅ search_input encontrado!");
		else
			Print("[AskalStore] ⚠️ search_input NÃO encontrado!");
		
	m_SearchButton = ButtonWidget.Cast(m_RootWidget.FindAnyWidget("search_button"));
	if (m_SearchButton)
		Print("[AskalStore] ✅ search_button encontrado!");
	else
		Print("[AskalStore] ⚠️ search_button NÃO encontrado (layout mantém visibilidade 0)!");
	
	// Painel de opções com toggles de compra/venda em lote
	Widget optionsPanel = m_RootWidget.FindAnyWidget("options_panel");
	if (optionsPanel)
	{
		Print("[AskalStore] ✅ options_panel encontrado!");
		
		m_BatchBuyToggle = ButtonWidget.Cast(optionsPanel.FindAnyWidget("toggle_batch_buy"));
		if (m_BatchBuyToggle)
		{
			Print("[AskalStore] ✅ toggle_batch_buy encontrado!");
			m_BatchBuyToggle.SetColor(ARGB(150, 0, 0, 0));
		}
		else
		{
			Print("[AskalStore] ⚠️ toggle_batch_buy NÃO encontrado!");
		}
		
		m_BatchSellToggle = ButtonWidget.Cast(optionsPanel.FindAnyWidget("toggle_batch_sell"));
		if (m_BatchSellToggle)
		{
			Print("[AskalStore] ✅ toggle_batch_sell encontrado!");
			m_BatchSellToggle.SetColor(ARGB(150, 0, 0, 0));
		}
		else
		{
			Print("[AskalStore] ⚠️ toggle_batch_sell NÃO encontrado!");
		}
	}
	else
	{
		Print("[AskalStore] ⚠️ options_panel NÃO encontrado!");
	}
	
	// Card especial do inventário do player (exibido no modo de venda em lote)
	m_PlayerInventoryCard = m_RootWidget.FindAnyWidget("player_Inventory_card");
	if (m_PlayerInventoryCard)
	{
		Print("[AskalStore] ✅ player_Inventory_card encontrado!");
		m_PlayerInventoryCard.Show(false); // Oculto por padrão
		m_PlayerInventoryHeaderButton = ButtonWidget.Cast(m_PlayerInventoryCard.FindAnyWidget("player_Inventory_header"));
		m_PlayerInventoryCategoryHolder = GridSpacerWidget.Cast(m_PlayerInventoryCard.FindAnyWidget("player_inventory_category_cards_holder"));
	}
	else
	{
		Print("[AskalStore] ⚠️ player_Inventory_card NÃO encontrado!");
	}
	
	// Carregar cooldown e hold time do config
	AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
	if (marketConfig)
	{
		float actionDelayMs = marketConfig.GetDelayTimeMS();
		if (actionDelayMs < 0)
			actionDelayMs = 0;
		m_CooldownDuration = actionDelayMs / 1000.0;
		Print("[AskalStore] ⏱️ Tempo de atraso configurado: " + actionDelayMs + "ms (" + m_CooldownDuration + "s)");
	}
	
	LoadVirtualStoreConfig();
	
	// Progress Bars de cooldown
	if (m_BuyButton)
	{
		m_BuyCooldownProgressBar = ProgressBarWidget.Cast(m_BuyButton.FindAnyWidget("buy_hold_pb"));
		if (!m_BuyCooldownProgressBar)
			m_BuyCooldownProgressBar = ProgressBarWidget.Cast(m_BuyButton.FindAnyWidget("buy_hold_pb_wide"));
		if (m_BuyCooldownProgressBar)
		{
			m_BuyCooldownProgressBar.SetCurrent(0);
			m_BuyCooldownProgressBar.Show(true);
			Print("[AskalStore] ✅ buy cooldown progress inicializado");
		}
	}
	if (m_SellButton)
	{
		m_SellCooldownProgressBar = ProgressBarWidget.Cast(m_SellButton.FindAnyWidget("sell_hold_pb"));
		if (!m_SellCooldownProgressBar)
			m_SellCooldownProgressBar = ProgressBarWidget.Cast(m_SellButton.FindAnyWidget("sell_hold_pb_wide"));
		if (m_SellCooldownProgressBar)
		{
			m_SellCooldownProgressBar.SetCurrent(0);
			m_SellCooldownProgressBar.Show(true);
			Print("[AskalStore] ✅ sell cooldown progress inicializado");
		}
	}
	if (m_BuyButtonSolo)
	{
		m_BuySoloCooldownProgressBar = ProgressBarWidget.Cast(m_BuyButtonSolo.FindAnyWidget("buy_only_hold_pb"));
		if (!m_BuySoloCooldownProgressBar)
			m_BuySoloCooldownProgressBar = ProgressBarWidget.Cast(m_BuyButtonSolo.FindAnyWidget("buy_hold_pb_wide"));
		if (m_BuySoloCooldownProgressBar)
		{
			m_BuySoloCooldownProgressBar.SetCurrent(0);
			m_BuySoloCooldownProgressBar.Show(true);
			Print("[AskalStore] ✅ buy solo cooldown progress inicializado");
		}
	}
	if (m_SellButtonSolo)
	{
		m_SellSoloCooldownProgressBar = ProgressBarWidget.Cast(m_SellButtonSolo.FindAnyWidget("sell_only_hold_pb"));
		if (!m_SellSoloCooldownProgressBar)
			m_SellSoloCooldownProgressBar = ProgressBarWidget.Cast(m_SellButtonSolo.FindAnyWidget("sell_hold_pb_wide"));
		if (m_SellSoloCooldownProgressBar)
		{
			m_SellSoloCooldownProgressBar.SetCurrent(0);
			m_SellSoloCooldownProgressBar.Show(true);
			Print("[AskalStore] ✅ sell solo cooldown progress inicializado");
		}
	}
	
	// Inicializar maps de inventário
		m_PlayerInventoryItems = new map<string, ref array<EntityAI>>();
		m_ItemHealthMap = new map<string, float>();
	m_ItemCardToInventoryItem = new map<Widget, EntityAI>();
	m_InventoryDisplayItems = new array<ref AskalInventoryDisplayInfo>();
	
	// Sistema de Notificações
		m_NotificationCardHolder = WrapSpacerWidget.Cast(m_RootWidget.FindAnyWidget("market_notification_card_holder"));
		if (m_NotificationCardHolder)
		{
			Print("[AskalStore] ✅ market_notification_card_holder encontrado!");
			// Limpar cards de exemplo do layout
			Widget child = m_NotificationCardHolder.GetChildren();
			while (child)
			{
				Widget next = child.GetSibling();
				delete child;
				child = next;
			}
			}
			else
			{
			Print("[AskalStore] ⚠️ market_notification_card_holder NÃO encontrado!");
		}
		
		if (!m_NotificationCards)
			m_NotificationCards = new array<Widget>();
		if (!m_NotificationTimestamps)
			m_NotificationTimestamps = new array<float>();
		
		// Painel de filtros adicionais
		//	Widget optionsPanel = m_RootWidget.FindAnyWidget("options_panel");
		//	if (optionsPanel)
		//	{
		//		m_SellableOnlyToggle = ThreeStateCheckboxWidget.Cast(optionsPanel.FindAnyWidget("sellableonly_toggle"));
		//		m_CompatibleOnlyToggle = ThreeStateCheckboxWidget.Cast(optionsPanel.FindAnyWidget("compatibleonly_toggle"));
		//		
		//		if (!m_SellableOnlyToggle)
		//			Print("[AskalStore] ⚠️ sellableonly_toggle NÃO encontrado!");
		//		if (!m_CompatibleOnlyToggle)
		//			Print("[AskalStore] ⚠️ compatibleonly_toggle NÃO encontrado!");
		//	}
		//	else
		//	{
		//		Print("[AskalStore] ⚠️ options_panel NÃO encontrado!");
		//	}
		
		// Breadcrumb e painel de transação
		m_ItemsBreadcrumbText = TextWidget.Cast(m_RootWidget.FindAnyWidget("items_section_breadcrumb_text"));
		Widget transactionPanel = m_RootWidget.FindAnyWidget("transaction_panel");
		if (transactionPanel)
		{
			m_TransactionDescription = TextWidget.Cast(transactionPanel.FindAnyWidget("transaction_description"));
			WrapSpacerWidget quantityPanel = WrapSpacerWidget.Cast(transactionPanel.FindAnyWidget("transaction_units_panel"));
			if (quantityPanel)
			{
				m_TransactionQuantityInput = EditBoxWidget.Cast(quantityPanel.FindAnyWidget("transaction_units_input"));
				m_TransactionQuantityPlusButton = ButtonWidget.Cast(quantityPanel.FindAnyWidget("transaction_units_plus_button"));
				m_TransactionQuantityMinusButton = ButtonWidget.Cast(quantityPanel.FindAnyWidget("transaction_units_minus_button"));
			}
			
			// Slider de Quantidade (oculto por padrão)
		m_TransactionQuantitySlider = SliderWidget.Cast(transactionPanel.FindAnyWidget("transaction_quantity_slider"));
		if (m_TransactionQuantitySlider)
		{
			m_TransactionQuantitySliderText = TextWidget.Cast(m_TransactionQuantitySlider.FindAnyWidget("transaction_quantity_slider_text"));
			m_TransactionQuantitySlider.Show(false); // Oculto por padrão
			Print("[AskalStore] ✅ Slider de quantidade inicializado");
			}
			else
			{
			Print("[AskalStore] ⚠️ transaction_quantity_slider NÃO encontrado!");
		}
		
		// Seletor de Conteúdo (oculto por padrão)
		m_TransactionContentPanel = transactionPanel.FindAnyWidget("transaction_content_panel");
		if (m_TransactionContentPanel)
		{
			m_TransactionContentLabel = TextWidget.Cast(m_TransactionContentPanel.FindAnyWidget("transaction_content_label"));
			m_TransactionContentPrevButton = ButtonWidget.Cast(m_TransactionContentPanel.FindAnyWidget("prev_content_button"));
			m_TransactionContentNextButton = ButtonWidget.Cast(m_TransactionContentPanel.FindAnyWidget("next_content_button"));
			m_TransactionContentPanel.Show(false); // Oculto por padrão
			Print("[AskalStore] ✅ Seletor de conteúdo inicializado");
		}
		else
		{
			Print("[AskalStore] ⚠️ transaction_content_panel NÃO encontrado!");
		}
	}
	else
	{
		Print("[AskalStore] ⚠️ transaction_panel NÃO encontrado!");
	}
		m_TransactionQuantity = 1;
		m_SelectedItemUnitPrice = 0;
		m_CurrentQuantityPercent = 100;
		m_CurrentAmmoCount = 0;
		
		// Inicializa arrays de conteúdo
		m_AvailableContentTypes = new array<string>();
		m_AvailableLiquidTypes = new array<int>();
		m_AvailableContentUnitPrices = new array<float>();
		m_CurrentContentIndex = 0;
		m_CurrentSelectedContent = "";
		m_CurrentItemIsLiquidContainer = false;
		m_CurrentLiquidCapacity = 0.0;
		RefreshTransactionQuantityDisplay();
		UpdateTransactionSummary();
		
		// Hover Text (NOVO LAYOUT)
		m_HoverPanel = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_hover_text.layout");
		if (m_HoverPanel)
		{
			m_HoverPanel.Show(false);
			m_HoverText = TextWidget.Cast(m_HoverPanel.FindAnyWidget("hover_tooltip_title_text"));
		}
		
		Print("[AskalStore] Widgets inicializados");
		
		// Carregar datasets dinamicamente do Core Database
		Print("[AskalStore] 🔄 Carregando datasets do Core Database...");
		LoadDatasetsFromCore();
		
		// Carregar primeira categoria e mostrar itens
		if (m_Datasets.Count() > 0)
		{
			LoadDataset(0);
			if (m_Categories.Count() > 0)
				LoadCategory(0);
		}
		else
		{
			Print("[AskalStore] ⚠️ Nenhum dataset encontrado no Core Database!");
		}
		
		// Escanear inventário do player
		ScanPlayerInventory();
		
		Print("[AskalStore] ================ Init() CONCLUÍDO ================");
		return m_RootWidget;
	}
	
	// ========================================
	// FUNÇÕES HELPER PARA FILTRAGEM DE TRADER
	// ========================================
	
	// Verificar se um dataset está disponível e retornar o modo
	// Retorna: -1 = disabled, 0 = see only, 1 = buy only, 2 = sell only, 3 = buy and sell
	// Prioridade: Item específico > Categoria > Dataset > ALL
	// Normalizar ID de dataset (adicionar prefixo DS_ se necessário)
	string NormalizeDatasetID(string datasetID)
	{
		if (!datasetID || datasetID == "")
			return "";
		
		// Se não tem prefixo DS_, adicionar
		if (datasetID.IndexOf("DS_") != 0)
			return "DS_" + datasetID;
		
		return datasetID;
	}
	
	// Normalizar ID de categoria (adicionar prefixo CAT_ se necessário)
	string NormalizeCategoryID(string categoryID)
	{
		if (!categoryID || categoryID == "")
			return "";
		
		// Se não tem prefixo CAT_, adicionar
		if (categoryID.IndexOf("CAT_") != 0)
			return "CAT_" + categoryID;
		
		return categoryID;
	}
	
	int GetDatasetMode(string datasetID)
	{
		if (!m_TraderSetupItems || m_TraderSetupItems.Count() == 0)
		{
			// Sem filtros, tudo disponível (modo 3 = buy and sell)
			return 3;
		}
		
		// Normalizar ID
		string normalizedID = NormalizeDatasetID(datasetID);
		
		// Verificar se há "ALL": 3 (todos os datasets disponíveis)
		int allMode = -1;
		if (m_TraderSetupItems.Contains("ALL"))
		{
			allMode = m_TraderSetupItems.Get("ALL");
		}
		
		// Verificar se há configuração específica para este dataset (DS_*)
		if (m_TraderSetupItems.Contains(normalizedID))
		{
			return m_TraderSetupItems.Get(normalizedID);
		}
		
		// Se "ALL" está definido, usar esse modo
		if (allMode >= 0)
		{
			return allMode;
		}
		
		// Sem configuração, não disponível
		return -1;
	}
	
	// Verificar se uma categoria está disponível e retornar o modo
	int GetCategoryMode(string datasetID, string categoryID)
	{
		if (!m_TraderSetupItems || m_TraderSetupItems.Count() == 0)
		{
			return 3; // Sem filtros, tudo disponível
		}
		
		// Normalizar IDs
		string normalizedDatasetID = NormalizeDatasetID(datasetID);
		string normalizedCategoryID = NormalizeCategoryID(categoryID);
		
		// Verificar categoria específica (CAT_*)
		if (m_TraderSetupItems.Contains(normalizedCategoryID))
		{
			return m_TraderSetupItems.Get(normalizedCategoryID);
		}
		
		// Verificar dataset (DS_*)
		int datasetMode = GetDatasetMode(normalizedDatasetID);
		if (datasetMode >= 0)
		{
			return datasetMode;
		}
		
		// Verificar "ALL"
		if (m_TraderSetupItems.Contains("ALL"))
		{
			return m_TraderSetupItems.Get("ALL");
		}
		
		return -1;
	}
	
	// Verificar se um item está disponível e retornar o modo
	int GetItemMode(string datasetID, string categoryID, string itemClassName)
	{
		if (!m_TraderSetupItems || m_TraderSetupItems.Count() == 0)
		{
			return 3; // Sem filtros, tudo disponível
		}
		
		// Normalizar IDs
		string normalizedDatasetID = NormalizeDatasetID(datasetID);
		string normalizedCategoryID = NormalizeCategoryID(categoryID);
		
		// Verificar item específico (className exato) - PRIORIDADE 1
		if (m_TraderSetupItems.Contains(itemClassName))
		{
			int itemMode = m_TraderSetupItems.Get(itemClassName);
			Print("[AskalStore] 🔍 GetItemMode: Item específico encontrado - " + itemClassName + " = " + itemMode);
			return itemMode;
		}
		
		// Verificar categoria (CAT_*) - PRIORIDADE 2
		int categoryMode = GetCategoryMode(normalizedDatasetID, normalizedCategoryID);
		if (categoryMode >= 0)
		{
			Print("[AskalStore] 🔍 GetItemMode: Categoria encontrada - " + normalizedCategoryID + " = " + categoryMode);
			return categoryMode;
		}
		
		// Verificar dataset (DS_*) - PRIORIDADE 3
		int datasetMode = GetDatasetMode(normalizedDatasetID);
		if (datasetMode >= 0)
		{
			Print("[AskalStore] 🔍 GetItemMode: Dataset encontrado - " + normalizedDatasetID + " = " + datasetMode);
			return datasetMode;
		}
		
		// Verificar "ALL" - PRIORIDADE 4
		if (m_TraderSetupItems.Contains("ALL"))
		{
			int allMode = m_TraderSetupItems.Get("ALL");
			Print("[AskalStore] 🔍 GetItemMode: ALL encontrado = " + allMode);
			return allMode;
		}
		
		Print("[AskalStore] 🔍 GetItemMode: Nenhuma configuração encontrada para " + itemClassName + " (DS: " + normalizedDatasetID + ", CAT: " + normalizedCategoryID + ")");
		return -1;
	}
	
	// Verificar se um dataset está disponível (não disabled)
	bool IsDatasetAvailable(string datasetID)
	{
		return GetDatasetMode(datasetID) >= 0;
	}
	
	// Verificar se uma categoria está disponível
	bool IsCategoryAvailable(string datasetID, string categoryID)
	{
		return GetCategoryMode(datasetID, categoryID) >= 0;
	}
	
	// Verificar se um item está disponível
	bool IsItemAvailable(string datasetID, string categoryID, string itemClassName)
	{
		return GetItemMode(datasetID, categoryID, itemClassName) >= 0;
	}
	
	// ========================================
	// CARREGAMENTO DO CACHE DO CLIENTE
	// ========================================
	
	void LoadDatasetsFromCore()
	{
		Print("[AskalStore] ========================================");
		Print("[AskalStore] LoadDatasetsFromCore() - CACHE SIMPLIFICADO");
		
		m_Datasets.Clear();
		
		// Verificar se está sincronizado
		if (GetGame().IsMultiplayer() && GetGame().IsClient())
		{
			if (!AskalDatabaseSync.IsClientSynced())
			{
				Print("[AskalStore] ⚠️ Database ainda não sincronizado!");
				Print("[AskalStore] Aguarde alguns segundos...");
			return;
		}
		}
		
		// Ler do cache
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		map<string, ref AskalDatasetSyncData> datasets = cache.GetDatasets();
		
		if (!datasets || datasets.Count() == 0)
		{
			Print("[AskalStore] ❌ Nenhum dataset no cache!");
			return;
		}
		
		Print("[AskalStore] ✅ Cache tem " + datasets.Count() + " datasets");
		
		// Adicionar datasets à lista (FILTRADOS pelo SetupItems do trader)
		int filteredCount = 0;
		for (int i = 0; i < datasets.Count(); i++)
		{
			string datasetID = datasets.GetKey(i);
			
			// Filtrar baseado no SetupItems do trader
			if (IsDatasetAvailable(datasetID))
			{
				m_Datasets.Insert(datasetID);
				int mode = GetDatasetMode(datasetID);
				Print("[AskalStore]   ✅ " + datasetID + " (modo: " + mode + ")");
			}
			else
			{
				filteredCount++;
				Print("[AskalStore]   ❌ " + datasetID + " (filtrado - não disponível)");
			}
		}
		
		if (filteredCount > 0)
		{
			Print("[AskalStore] 📊 " + filteredCount + " datasets filtrados (não disponíveis para este trader)");
		}
		
		// Criar cards de datasets
		CreateDatasetCards();
		
		Print("[AskalStore] ✅ " + m_Datasets.Count() + " datasets carregados");
		Print("[AskalStore] ========================================");
	}
	
	void LoadDataset(int index)
	{
		if (index < 0 || index >= m_Datasets.Count())
		{
			Print("[AskalStore] ❌ Índice inválido: " + index);
			return;
		}
		
		string datasetID = m_Datasets.Get(index);
		int previousDatasetIndex = m_CurrentDatasetIndex;
		m_CurrentDatasetIndex = index;
		if (previousDatasetIndex != -1 && previousDatasetIndex != index)
		{
			ResetDatasetCategoryHighlights(previousDatasetIndex);
		}
		
		Print("[AskalStore] ========================================");
		Print("[AskalStore] LoadDataset(" + index + ") → " + datasetID);
		
		// Ler do cache
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		AskalDatasetSyncData dataset = cache.GetDataset(datasetID);
		
		if (!dataset)
		{
			Print("[AskalStore] ❌ Dataset não encontrado no cache: " + datasetID);
			return;
		}
		
		Print("[AskalStore] ✅ Dataset: " + dataset.DisplayName);
		Print("[AskalStore]    Categorias: " + dataset.Categories.Count());
		
		// Garantir que as categorias estão construídas para este dataset
		BuildCategoriesForDataset(index, dataset);
		HighlightDatasetCard(index);
		SetDatasetExpanded(index, true);
		
		// Atualizar arrays globais para o dataset atual
		m_Categories.Clear();
		m_CategoryDisplayNames.Clear();
		array<string> categoryIDsForDataset = m_DatasetCategoryIDsPerDataset.Get(index);
		array<string> categoryNamesForDataset = m_DatasetCategoryDisplayNamesPerDataset.Get(index);
		for (int catIdx = 0; catIdx < categoryIDsForDataset.Count(); catIdx++)
		{
			m_Categories.Insert(categoryIDsForDataset.Get(catIdx));
			if (catIdx < categoryNamesForDataset.Count())
			{
				m_CategoryDisplayNames.Insert(categoryNamesForDataset.Get(catIdx));
		}
		else
		{
				m_CategoryDisplayNames.Insert(categoryIDsForDataset.Get(catIdx));
			}
		}
		
		m_CategoryCardRoots = m_DatasetCategoryCardRootsPerDataset.Get(index);
		m_CategorySelectedButtons = m_DatasetCategoryButtonsPerDataset.Get(index);
		m_CategoryUnselectedButtons = m_DatasetCategoryButtonsPerDataset.Get(index);
		m_CategorySelectedTexts = m_DatasetCategoryTextsPerDataset.Get(index);
		m_CategoryUnselectedTexts = m_DatasetCategoryTextsPerDataset.Get(index);
		m_CurrentCategoryIndex = 0;
		UpdateCategoryCardsVisual();
		
			Print("[AskalStore] ========================================");
		}
		
	void LoadCategory(int index)
		{
		if (index < 0 || index >= m_Categories.Count())
		{
			Print("[AskalStore] ❌ Índice de categoria inválido: " + index);
			return;
		}
		
		m_CurrentCategoryIndex = index;
		string categoryID = m_Categories.Get(index);
		
		Print("[AskalStore] ========================================");
		Print("[AskalStore] LoadCategory(" + index + ") → " + categoryID);
		
		// Ler do cache
		string datasetID = m_Datasets.Get(m_CurrentDatasetIndex);
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		AskalDatasetSyncData dataset = cache.GetDataset(datasetID);
		
		if (!dataset)
		{
			Print("[AskalStore] ❌ Dataset não encontrado");
			return;
		}
		
		AskalCategorySyncData category = dataset.Categories.Get(categoryID);
		if (!category)
		{
			Print("[AskalStore] ❌ Categoria não encontrada");
			return;
		}
		
		Print("[AskalStore] ✅ Categoria: " + category.DisplayName);
		Print("[AskalStore]    Items: " + category.Items.Count());
		
		// Atualizar breadcrumb (dataset > categoria)
		string datasetDisplayName = "";
		if (m_CurrentDatasetIndex >= 0 && m_CurrentDatasetIndex < m_DatasetDisplayNames.Count())
			datasetDisplayName = m_DatasetDisplayNames.Get(m_CurrentDatasetIndex);
		UpdateBreadcrumb(datasetDisplayName, category.DisplayName);
		
		// Limpar items antigos
		ClearItemCards();
		m_Items.Clear();
		if (m_ItemDatasetIds)
			m_ItemDatasetIds.Clear();
		if (m_ItemCategoryIds)
			m_ItemCategoryIds.Clear();
		
		ref map<string, bool> processedClasses = new map<string, bool>();
		ref map<string, bool> variantClassLookup = new map<string, bool>();
		for (int variantIdx = 0; variantIdx < category.Items.Count(); variantIdx++)
		{
			string variantSourceClass = category.Items.GetKey(variantIdx);
			AskalItemSyncData variantSourceData = category.Items.Get(variantSourceClass);
			if (!variantSourceData || !variantSourceData.Variants || variantSourceData.Variants.Count() == 0)
				continue;
			foreach (string variantClassName : variantSourceData.Variants)
			{
				if (variantClassName && variantClassName != "")
				{
					variantClassLookup.Set(variantClassName, true);
				}
			}
		}
		array<string> orderedItemIDs = new array<string>();
		for (int itemMainIdx = 0; itemMainIdx < category.Items.Count(); itemMainIdx++)
		{
			string candidateClassName = category.Items.GetKey(itemMainIdx);
			if (variantClassLookup.Contains(candidateClassName))
			{
				Print("[AskalStore] 🔁 Ignorando variante na lista principal: " + candidateClassName);
				continue;
			}
			orderedItemIDs.Insert(candidateClassName);
		}
		
		// Filtrar itens baseado no SetupItems do trader
		for (int orderedIdx = 0; orderedIdx < orderedItemIDs.Count(); orderedIdx++)
		{
			string classNameLoop = orderedItemIDs.Get(orderedIdx);
			
			// Verificar se o item está disponível para este trader
			// Modo -1 = disabled (não aparece), modo 0+ = aparece (mas botões podem estar desabilitados)
			int itemMode = GetItemMode(datasetID, categoryID, classNameLoop);
			if (itemMode < 0) // Modo -1 = disabled, não aparece na lista
			{
				Print("[AskalStore] 🔒 Item filtrado (disabled): " + classNameLoop);
				continue;
			}
			
			AskalItemSyncData syncItemLoop = category.Items.Get(classNameLoop);
			AddItemEntryForCategory(datasetID, categoryID, category, classNameLoop, syncItemLoop, processedClasses, cache, true);
			
			// Popular cache de dataset/categoria para performance
			if (!m_ItemToDatasetCategoryCache.Contains(classNameLoop))
			{
				m_ItemToDatasetCategoryCache.Set(classNameLoop, new Param2<string, string>(datasetID, categoryID));
			}
		}
		
		// Renderizar items (incluindo variantes)
		RenderItems();
		
		// Atualizar category cards visual
		UpdateCategoryCardsVisual();
		
		Print("[AskalStore] ✅ " + m_Items.Count() + " items carregados");
		Print("[AskalStore] ========================================");
	}
	
		// ========================================
	// DATASET CARDS
		// ========================================
	
	void CreateDatasetCards()
	{
		Print("[AskalStore] CreateDatasetCards() - Total: " + m_Datasets.Count());
		
		if (!m_DatasetCardsHolder)
		{
			Print("[AskalStore] ❌ DatasetCardsHolder é NULL!");
			return;
		}
		
		// Limpar cards anteriores
		Widget playerInventoryCard = m_PlayerInventoryCard;
		Widget child = m_DatasetCardsHolder.GetChildren();
		while (child)
		{
			Widget next = child.GetSibling();
			if (child != playerInventoryCard)
			delete child;
			child = next;
		}
		
		m_DatasetButtons.Clear();
		m_DatasetCardRoots.Clear();
		m_DatasetIconWidgets.Clear();
		m_DatasetDisplayNames.Clear();
		m_DatasetExpandedStates.Clear();
		m_DatasetCategoriesBuilt.Clear();
		m_DatasetCategoryHolders.Clear();
		m_DatasetCategoryCardRootsPerDataset.Clear();
		m_DatasetCategoryButtonsPerDataset.Clear();
		m_DatasetCategoryTextsPerDataset.Clear();
		m_DatasetCategoryIDsPerDataset.Clear();
		m_DatasetCategoryDisplayNamesPerDataset.Clear();
		
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (!cache)
		{
			Print("[AskalStore] ⚠️ Cache não disponível ao criar dataset cards");
		}
		
		for (int datasetIdx = 0; datasetIdx < m_Datasets.Count(); datasetIdx++)
		{
			string datasetID = m_Datasets.Get(datasetIdx);
			AskalDatasetSyncData datasetData = NULL;
			if (cache)
			{
				datasetData = cache.GetDataset(datasetID);
			}
			
			Widget datasetCardRoot = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_dataset_card.layout", m_DatasetCardsHolder);
			if (!datasetCardRoot)
			{
				Print("[AskalStore] ❌ Falha ao criar dataset card para: " + datasetID);
				m_DatasetButtons.Insert(NULL);
				m_DatasetCardRoots.Insert(NULL);
				m_DatasetIconWidgets.Insert(NULL);
				m_DatasetDisplayNames.Insert(datasetID);
				m_DatasetExpandedStates.Insert(true);
				m_DatasetCategoriesBuilt.Insert(false);
				m_DatasetCategoryHolders.Insert(NULL);
				m_DatasetCategoryCardRootsPerDataset.Insert(new array<Widget>());
				m_DatasetCategoryButtonsPerDataset.Insert(new array<ButtonWidget>());
				m_DatasetCategoryTextsPerDataset.Insert(new array<TextWidget>());
				m_DatasetCategoryIDsPerDataset.Insert(new array<string>());
				m_DatasetCategoryDisplayNamesPerDataset.Insert(new array<string>());
				continue;
			}
			
			m_DatasetCardRoots.Insert(datasetCardRoot);
			
			ButtonWidget headerButton = ButtonWidget.Cast(datasetCardRoot.FindAnyWidget("dataset_card_header"));
			TextWidget datasetNameText = TextWidget.Cast(datasetCardRoot.FindAnyWidget("dataset_card_name_text"));
			GridSpacerWidget categoryHolder = GridSpacerWidget.Cast(datasetCardRoot.FindAnyWidget("category_cards_holder"));
			ImageWidget datasetIconWidget = ImageWidget.Cast(datasetCardRoot.FindAnyWidget("dataset_card_icon"));
			
			string datasetDisplayName = datasetID;
			if (datasetData && datasetData.DisplayName != "")
			{
				datasetDisplayName = datasetData.DisplayName;
			}
			
			if (datasetNameText)
			{
				datasetNameText.SetText(datasetDisplayName);
				}
				else
				{
				Print("[AskalStore] ⚠️ dataset_card_name_text não encontrado para: " + datasetID);
			}
			string datasetIconPath = DEFAULT_DATASET_ICON;
			if (datasetData && datasetData.Icon != "")
				datasetIconPath = datasetData.Icon;
			if (datasetIconWidget)
			{
				datasetIconWidget.LoadImageFile(0, datasetIconPath);
				datasetIconWidget.Show(true);
			}
			else
			{
				Print("[AskalStore] ⚠️ dataset_card_icon não encontrado para: " + datasetID);
			}
			m_DatasetIconWidgets.Insert(datasetIconWidget);
			
			if (headerButton)
			{
				m_DatasetButtons.Insert(headerButton);
				}
				else
				{
				Print("[AskalStore] ⚠️ dataset_card_header não encontrado para: " + datasetID);
				m_DatasetButtons.Insert(NULL);
			}
			
			if (categoryHolder)
			{
				Widget categoryChild = categoryHolder.GetChildren();
				while (categoryChild)
				{
					Widget nextCategory = categoryChild.GetSibling();
					delete categoryChild;
					categoryChild = nextCategory;
				}
				}
				else
				{
				Print("[AskalStore] ⚠️ category_cards_holder não encontrado no dataset card: " + datasetID);
			}
			
			m_DatasetDisplayNames.Insert(datasetDisplayName);
			m_DatasetExpandedStates.Insert(true);
			m_DatasetCategoriesBuilt.Insert(false);
			m_DatasetCategoryHolders.Insert(categoryHolder);
			
			ref array<Widget> datasetCategoryRoots = new array<Widget>();
			ref array<ButtonWidget> datasetCategoryButtons = new array<ButtonWidget>();
			ref array<TextWidget> datasetCategoryTexts = new array<TextWidget>();
			ref array<string> datasetCategoryIDs = new array<string>();
			ref array<string> datasetCategoryDisplayNames = new array<string>();
			m_DatasetCategoryCardRootsPerDataset.Insert(datasetCategoryRoots);
			m_DatasetCategoryButtonsPerDataset.Insert(datasetCategoryButtons);
			m_DatasetCategoryTextsPerDataset.Insert(datasetCategoryTexts);
			m_DatasetCategoryIDsPerDataset.Insert(datasetCategoryIDs);
			m_DatasetCategoryDisplayNamesPerDataset.Insert(datasetCategoryDisplayNames);
			
			if (datasetData)
			{
				BuildCategoriesForDataset(datasetIdx, datasetData);
				}
				else
				{
				Print("[AskalStore] ⚠️ Dados do dataset não encontrados no cache: " + datasetID);
			}
			
			Print("[AskalStore] ✅ Dataset card criado: " + datasetDisplayName);
		}
		
		m_DatasetCardsHolder.Update();
		HighlightDatasetCard(m_CurrentDatasetIndex);
	}
	
	void ClearItemCards()
	{
		// Limpar widgets de itens
		foreach (Widget widget : m_ItemWidgets)
		{
			if (widget)
				delete widget;
		}
		m_ItemWidgets.Clear();
		m_ItemCardToIndex.Clear();
	}
	
	void UpdateCategoryCardsVisual()
	{
		// Atualizar visual dos category cards (destacar selecionado)
		HighlightCategoryCard(m_CurrentCategoryIndex);
	}
	
	// ========================================
	// CARREGAMENTO E RENDERIZAÇÃO
	// ========================================
	

	void BuildCategoriesForDataset(int datasetIdx, AskalDatasetSyncData datasetData)
	{
		if (datasetIdx < 0 || datasetIdx >= m_DatasetCardRoots.Count())
		{
			Print("[AskalStore] ❌ BuildCategoriesForDataset: índice inválido " + datasetIdx);
			return;
		}
		
		GridSpacerWidget categoryHolder = NULL;
		if (datasetIdx < m_DatasetCategoryHolders.Count())
			categoryHolder = m_DatasetCategoryHolders.Get(datasetIdx);
		
		if (!categoryHolder)
		{
			Print("[AskalStore] ❌ BuildCategoriesForDataset: category_holder é NULL para dataset " + datasetIdx);
			return;
		}
		
		ref array<Widget> categoryRoots = m_DatasetCategoryCardRootsPerDataset.Get(datasetIdx);
		ref array<ButtonWidget> categoryButtons = m_DatasetCategoryButtonsPerDataset.Get(datasetIdx);
		ref array<TextWidget> categoryTexts = m_DatasetCategoryTextsPerDataset.Get(datasetIdx);
		ref array<string> categoryIDs = m_DatasetCategoryIDsPerDataset.Get(datasetIdx);
		ref array<string> categoryDisplayNames = m_DatasetCategoryDisplayNamesPerDataset.Get(datasetIdx);
		
		categoryRoots.Clear();
		categoryButtons.Clear();
		categoryTexts.Clear();
		categoryIDs.Clear();
		categoryDisplayNames.Clear();
		
		Widget existingChild = categoryHolder.GetChildren();
		while (existingChild)
		{
			Widget nextChild = existingChild.GetSibling();
			delete existingChild;
			existingChild = nextChild;
		}
		
		if (!datasetData)
		{
			Print("[AskalStore] ⚠️ BuildCategoriesForDataset: datasetData é NULL para índice " + datasetIdx);
			m_DatasetCategoriesBuilt.Set(datasetIdx, false);
			return;
		}
		
		ref array<string> orderedIDs = new array<string>();
		if (datasetData.CategoryOrder && datasetData.CategoryOrder.Count() > 0)
		{
			for (int orderIdx = 0; orderIdx < datasetData.CategoryOrder.Count(); orderIdx++)
			{
				string orderedId = datasetData.CategoryOrder.Get(orderIdx);
				if (orderedId && datasetData.Categories && datasetData.Categories.Contains(orderedId))
					orderedIDs.Insert(orderedId);
			}
		}
		
		if (orderedIDs.Count() == 0 && datasetData.Categories)
		{
			for (int fallbackIdx = 0; fallbackIdx < datasetData.Categories.Count(); fallbackIdx++)
			{
				string fallbackId = datasetData.Categories.GetKey(fallbackIdx);
				if (fallbackId)
					orderedIDs.Insert(fallbackId);
			}
		}
		
		Print("[AskalStore] BuildCategoriesForDataset(" + datasetIdx + ") - Total: " + orderedIDs.Count());
		
		// Obter datasetID para filtragem
		string datasetID = "";
		if (datasetIdx >= 0 && datasetIdx < m_Datasets.Count())
		{
			datasetID = m_Datasets.Get(datasetIdx);
		}
		
		for (int i = 0; i < orderedIDs.Count(); i++)
		{
			string categoryID = orderedIDs.Get(i);
			
			// Filtrar categoria baseado no SetupItems do trader
			if (!IsCategoryAvailable(datasetID, categoryID))
			{
				Print("[AskalStore] 🔒 Categoria filtrada (não disponível): " + categoryID);
				continue;
			}
			
			AskalCategorySyncData categoryData = datasetData.Categories.Get(categoryID);
			string displayName = categoryID;
			if (categoryData && categoryData.DisplayName != "")
			{
				displayName = categoryData.DisplayName;
			}
			
			Widget categoryCardRoot = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_category_card.layout", categoryHolder);
			if (!categoryCardRoot)
			{
				Print("[AskalStore] ❌ Falha ao criar category card: " + categoryID);
				continue;
			}
			
			ButtonWidget categoryButton = ButtonWidget.Cast(categoryCardRoot.FindAnyWidget("category_card_button"));
			TextWidget categoryNameText = TextWidget.Cast(categoryCardRoot.FindAnyWidget("category_card_name_text"));
			if (categoryNameText)
			{
				categoryNameText.SetText(displayName);
			}
			else
			{
				Print("[AskalStore] ⚠️ category_card_name_text não encontrado para: " + categoryID);
			}
			
			categoryRoots.Insert(categoryCardRoot);
			categoryButtons.Insert(categoryButton);
			categoryTexts.Insert(categoryNameText);
			categoryIDs.Insert(categoryID);
			categoryDisplayNames.Insert(displayName);
			
			Print("[AskalStore] ✅ Category card criado: " + displayName);
		}
		
		categoryHolder.Show(m_DatasetExpandedStates.Get(datasetIdx));
		categoryHolder.Update();
		Widget datasetCardRoot = m_DatasetCardRoots.Get(datasetIdx);
		if (datasetCardRoot)
			datasetCardRoot.Update();
		m_DatasetCardsHolder.Update();
		m_DatasetCategoriesBuilt.Set(datasetIdx, true);
	ResetDatasetCategoryHighlights(datasetIdx);
	}

	void SetDatasetExpanded(int datasetIdx, bool expanded)
	{
		if (datasetIdx < 0 || datasetIdx >= m_DatasetExpandedStates.Count())
			return;
		
		m_DatasetExpandedStates.Set(datasetIdx, expanded);
		
		if (datasetIdx < m_DatasetCategoryHolders.Count())
		{
			GridSpacerWidget holder = m_DatasetCategoryHolders.Get(datasetIdx);
			if (holder)
			{
				holder.Show(expanded);
				holder.Update();
			}
		}
		
		if (m_DatasetCardsHolder)
			m_DatasetCardsHolder.Update();
	}

	void ResetDatasetCategoryHighlights(int datasetIdx)
	{
		if (datasetIdx < 0 || datasetIdx >= m_DatasetCategoryTextsPerDataset.Count())
							return;
		
		array<TextWidget> texts = m_DatasetCategoryTextsPerDataset.Get(datasetIdx);
		array<ButtonWidget> buttons = m_DatasetCategoryButtonsPerDataset.Get(datasetIdx);
		
		if (texts)
		{
			for (int i = 0; i < texts.Count(); i++)
			{
				TextWidget textWidget = texts.Get(i);
				if (textWidget)
					textWidget.SetColor(ARGB(200, 200, 200, 200));
				
				ButtonWidget buttonWidget = NULL;
				if (buttons && i < buttons.Count())
					buttonWidget = buttons.Get(i);
				
				if (buttonWidget)
					buttonWidget.SetColor(ARGB(255, 60, 60, 60));
			}
		}
	}

	void HighlightDatasetCard(int activeIndex)
	{
		for (int datasetIdx = 0; datasetIdx < m_DatasetCardRoots.Count(); datasetIdx++)
		{
			Widget datasetRoot = m_DatasetCardRoots.Get(datasetIdx);
			if (!datasetRoot)
				continue;
			
			ButtonWidget headerButton = ButtonWidget.Cast(datasetRoot.FindAnyWidget("dataset_card_header"));
			TextWidget datasetNameText = TextWidget.Cast(datasetRoot.FindAnyWidget("dataset_card_name_text"));
			
			if (headerButton)
			{
				if (datasetIdx == activeIndex)
					headerButton.SetColor(ARGB(255, 110, 110, 110));
				else
					headerButton.SetColor(ARGB(255, 60, 60, 60));
			}
			
			if (datasetNameText)
			{
				if (datasetIdx == activeIndex)
					datasetNameText.SetColor(ARGB(255, 255, 255, 255));
				else
					datasetNameText.SetColor(ARGB(200, 200, 200, 200));
			}
		}
	}
	
	void HighlightCategoryCard(int activeIndex)
	{
		ResetDatasetCategoryHighlights(m_CurrentDatasetIndex);
		
		if (activeIndex < 0 || activeIndex >= m_CategorySelectedTexts.Count())
					return;
		
		TextWidget categoryText = m_CategorySelectedTexts.Get(activeIndex);
		ButtonWidget categoryButton = NULL;
		if (activeIndex < m_CategorySelectedButtons.Count())
			categoryButton = m_CategorySelectedButtons.Get(activeIndex);
		
		if (categoryText)
			categoryText.SetColor(ARGB(255, 255, 255, 255));
		
		if (categoryButton)
			categoryButton.SetColor(ARGB(255, 100, 100, 100));
	}
	
	void RenderItems()
	{
		Print("[AskalStore] RenderItems() - Total: " + m_Items.Count());
		
		if (!m_ItensCardWrap)
		{
			Print("[AskalStore] ❌ m_ItensCardWrap é NULL!");
			return;
		}
		
		if (m_ShowingInventoryForSale)
		{
			Print("[AskalStore] 📦 Renderizando itens do inventário para venda em lote");
			RenderInventoryItemsForSale();
			return;
		}
		
		// Limpar itens anteriores
		ClearItems();
		
		// Criar cards para cada item
		for (int i = 0; i < m_Items.Count(); i++)
		{
			AskalItemData itemData = m_Items.Get(i);
			
			// FILTRO: Sellable Only
			if (m_BatchSellEnabled && !IsItemInInventory(itemData.GetClassName()))
			{
				Print("[AskalStore] 🔍 Filtro Sellable Only: " + itemData.GetClassName() + " NÃO está no inventário, pulando...");
				continue;
			}
			
			// Criar card do item
			Widget itemCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_item_card.layout", m_ItensCardWrap);
			
			if (!itemCard)
				continue;
			
			// Configurar nome
			TextWidget nameWidget = MultilineTextWidget.Cast(itemCard.FindAnyWidget("item_card_name_text"));
				if (nameWidget)
				{
				string displayName = itemData.GetDisplayName();
					if (displayName == "")
					displayName = itemData.GetClassName();
					nameWidget.SetText(displayName);
				}
				
				// Configurar preço
			TextWidget priceWidget = TextWidget.Cast(itemCard.FindAnyWidget("item_card_price_text"));
				if (priceWidget)
			{
				string formattedPrice = FormatCurrencyValue(itemData.GetPrice());
				priceWidget.SetText(formattedPrice);
			}
			
			// RENDERIZAR MINIATURA 3D NO CARD
			// O preview está dentro de item_preview_panel
			Widget previewPanel = itemCard.FindAnyWidget("item_preview_panel");
			ItemPreviewWidget cardPreview = NULL;
			if (previewPanel)
			{
				cardPreview = ItemPreviewWidget.Cast(previewPanel.FindAnyWidget("item_preview_widget"));
				if (!cardPreview)
					Print("[AskalStore] ⚠️ item_preview_widget não encontrado dentro de item_preview_panel para: " + itemData.GetClassName());
			}
			else
			{
				Print("[AskalStore] ⚠️ item_preview_panel não encontrado para: " + itemData.GetClassName());
			}
			
			if (!cardPreview)
			{
				cardPreview = ItemPreviewWidget.Cast(itemCard.FindAnyWidget("item_preview_widget"));
				if (!cardPreview)
					Print("[AskalStore] ⚠️ item_preview_widget não encontrado diretamente no card para: " + itemData.GetClassName());
			}
			
			if (cardPreview)
			{
				EntityAI previewItem = EntityAI.Cast(SpawnTemporaryObject(itemData.GetClassName()));
				if (previewItem)
				{
					ApplyDefaultAttachmentsToEntity(previewItem, itemData.GetDefaultAttachments());
					cardPreview.SetItem(previewItem);
					cardPreview.SetModelPosition(Vector(0, 0, 0.5));
					cardPreview.SetModelOrientation(Vector(0, 0, 0));
					cardPreview.SetView(previewItem.GetViewIndex());
					cardPreview.Show(true);
					m_PreviewItems.Insert(previewItem);
					Print("[AskalStore] ✅ Preview 3D criado no card: " + itemData.GetClassName());
										}
										else
										{
					Print("[AskalStore] ❌ Falha ao criar entidade para preview: " + itemData.GetClassName());
				}
			}
			else
			{
				Print("[AskalStore] ❌ item_preview_widget não encontrado em nenhum lugar para: " + itemData.GetClassName());
			}
			
			// INDICADOR DE INVENTÁRIO (ícone colorido por durabilidade)
			EntityAI inventoryItemForCard = null;
			ImageWidget inventoryIcon = ImageWidget.Cast(itemCard.FindAnyWidget("item_card_in_Inventory"));
			if (inventoryIcon)
			{
				if (IsItemInInventory(itemData.GetClassName()))
				{
					EntityAI inventoryItem = GetFirstItemInInventory(itemData.GetClassName());
					if (inventoryItem)
					{
					inventoryItemForCard = inventoryItem;
					inventoryIcon.Show(true);
					
					// Obter health do servidor (via RPC) ou usar 100% como fallback
					float health = GetItemHealth(itemData.GetClassName());
					AskalColorHelper.ApplyHealthColor(inventoryIcon, health);
						
						Print("[AskalStore] 🎨 Indicador de inventário: " + itemData.GetClassName() + " | Health: " + health + "%");
					}
					else
					{
						inventoryIcon.Show(false);
					}
				}
				else
				{
					inventoryIcon.Show(false);
				}
			}
			
			// Mapear card ao item real do inventário (para venda)
			if (m_ItemCardToInventoryItem)
			{
				if (inventoryItemForCard)
				{
					m_ItemCardToInventoryItem.Set(itemCard, inventoryItemForCard);
					Print("[AskalStore] 📦 Card mapeado ao item do inventário: " + itemData.GetClassName());
				}
				else if (m_ItemCardToInventoryItem.Contains(itemCard))
				{
					m_ItemCardToInventoryItem.Remove(itemCard);
				}
			}
			
			// Mapear card ao índice
			m_ItemCardToIndex.Set(itemCard, i);
			m_ItemWidgets.Insert(itemCard);
			
			Print("[AskalStore] ✅ Card criado: " + itemData.GetClassName());
		}
		
		// FIX UpdateOffset: Atualizar containers após criar widgets
		if (m_ItensCardWrap)
		{
			m_ItensCardWrap.Update();
			Print("[AskalStore] ✅ ItensCardWrap atualizado");
		}
		if (m_ItensCardScroll)
		{
			m_ItensCardScroll.Update();
			Print("[AskalStore] ✅ ItensCardScroll atualizado");
		}
		
		Print("[AskalStore] ✅ Renderização concluída!");
	}
	
	void RenderInventoryItemsForSale()
	{
		if (!m_ItensCardWrap)
			return;
		
		ClearItems();
		
		if (!m_PlayerInventoryItems)
		{
			Print("[AskalStore] ⚠️ m_PlayerInventoryItems é NULL, nada para renderizar");
			return;
		}
		
		if (!m_InventoryDisplayItems)
			m_InventoryDisplayItems = new array<ref AskalInventoryDisplayInfo>();
		m_InventoryDisplayItems.Clear();
		
		int totalCards = 0;
		for (int mapIdx = 0; mapIdx < m_PlayerInventoryItems.Count(); mapIdx++)
		{
			string normalizedClass = m_PlayerInventoryItems.GetKey(mapIdx);
			array<EntityAI> itemsOfClass = m_PlayerInventoryItems.GetElement(mapIdx);
			if (!itemsOfClass)
				continue;
			
			foreach (EntityAI invItem : itemsOfClass)
			{
				if (!invItem)
					continue;
				
				string className = invItem.GetType();
				if (!IsItemSellableByTrader(className))
				{
					Print("[AskalStore] ⚠️ Item do inventário não é negociável: " + className);
					continue;
				}
				
				AskalInventoryDisplayInfo info = new AskalInventoryDisplayInfo();
				info.Item = invItem;
				info.ClassName = className;
				
				ItemBase itemBase = ItemBase.Cast(invItem);
				string fallbackDisplay = "";
				if (itemBase)
					fallbackDisplay = itemBase.GetDisplayName();
				info.DisplayName = ResolveItemDisplayName(className, fallbackDisplay);
				if (!info.DisplayName || info.DisplayName == "")
					info.DisplayName = className;
				
				// Obter health do servidor (via RPC) ou usar 100% como fallback
				info.HealthPercent = GetItemHealth(className);
				info.EstimatedPrice = ComputeSellEstimate(invItem);
				
				m_InventoryDisplayItems.Insert(info);
				
				Widget itemCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_item_card.layout", m_ItensCardWrap);
				if (!itemCard)
					continue;
				
				totalCards++;
				
				TextWidget nameWidget = MultilineTextWidget.Cast(itemCard.FindAnyWidget("item_card_name_text"));
				if (nameWidget)
					nameWidget.SetText(info.DisplayName);
				
				TextWidget priceWidget = TextWidget.Cast(itemCard.FindAnyWidget("item_card_price_text"));
				if (priceWidget)
				{
					string formattedEstimate = FormatCurrencyValue(info.EstimatedPrice);
					priceWidget.SetText(formattedEstimate);
				}
				
				ItemPreviewWidget cardPreview = ItemPreviewWidget.Cast(itemCard.FindAnyWidget("item_preview_widget"));
				if (cardPreview)
				{
					EntityAI previewEntity = EntityAI.Cast(SpawnTemporaryObject(className));
					if (previewEntity)
					{
						CopyInventoryAttachmentsToPreview(invItem, previewEntity);
						cardPreview.SetItem(previewEntity);
						cardPreview.SetModelPosition(Vector(0, 0, 0.5));
						cardPreview.SetModelOrientation(Vector(0, 0, 0));
						cardPreview.SetView(previewEntity.GetViewIndex());
						cardPreview.Show(true);
						m_PreviewItems.Insert(previewEntity);
					}
				}
				
				ImageWidget inventoryIcon = ImageWidget.Cast(itemCard.FindAnyWidget("item_card_in_Inventory"));
				if (inventoryIcon)
				{
					inventoryIcon.Show(true);
					AskalColorHelper.ApplyHealthColor(inventoryIcon, info.HealthPercent);
				}
				
				m_ItemCardToInventoryItem.Set(itemCard, invItem);
				int infoIndex = m_InventoryDisplayItems.Count() - 1;
				m_ItemCardToIndex.Set(itemCard, infoIndex);
				m_ItemWidgets.Insert(itemCard);
				
				if (m_BatchSellSelectedEntities && m_BatchSellSelectedEntities.Find(invItem) != -1)
					SetCardSelected(itemCard, true);
				else
					SetCardSelected(itemCard, false);
			}
		}
		
		if (m_ItensCardWrap)
			m_ItensCardWrap.Update();
		if (m_ItensCardScroll)
			m_ItensCardScroll.Update();
		
		Print("[AskalStore] ✅ Inventário renderizado para venda: " + totalCards + " cards");
		
		if (m_BatchSellEnabled)
		{
			if (totalCards == 0)
				DisplayTransactionError("Nenhum item vendável encontrado no inventário");
			else
				DisplayTransactionMessage("Selecione itens do inventário para vender em lote");
		}
	}
	
	protected bool IsItemSellableByTrader(string className)
	{
		if (!className || className == "")
			return false;
		
		string normalizedClass = NormalizeClassName(className);
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (!cache)
			return false;
		
		AskalItemSyncData syncData = cache.FindItem(className);
		if (!syncData && normalizedClass != className)
			syncData = cache.FindItem(normalizedClass);
		return syncData != NULL;
	}
	
	protected void CopyInventoryAttachmentsToPreview(EntityAI source, EntityAI preview)
	{
		if (!source || !preview)
			return;
		if (!source.GetInventory() || !preview.GetInventory())
			return;
		
		// Separar magazines de outros attachments
		array<string> magazines = new array<string>();
		array<string> otherAttachments = new array<string>();
		
		int attachmentCount = source.GetInventory().AttachmentCount();
		for (int idx = 0; idx < attachmentCount; idx++)
		{
			EntityAI sourceAttachment = source.GetInventory().GetAttachmentFromIndex(idx);
			if (!sourceAttachment)
				continue;
			
			string attachmentClass = sourceAttachment.GetType();
			
			// Verificar se é um magazine
			string configPath = "CfgMagazines " + attachmentClass;
			if (GetGame().ConfigIsExisting(configPath))
			{
				magazines.Insert(attachmentClass);
			}
			else
			{
				otherAttachments.Insert(attachmentClass);
			}
		}
		
		// Aplicar outros attachments primeiro
		foreach (string attachClass : otherAttachments)
		{
			EntityAI previewAttachment = EntityAI.Cast(preview.GetInventory().CreateAttachment(attachClass));
			if (previewAttachment)
			{
				// Encontrar o attachment correspondente no source para copiar attachments aninhados
				EntityAI matchingSourceAttachment = NULL;
				for (int i = 0; i < source.GetInventory().AttachmentCount(); i++)
				{
					EntityAI att = source.GetInventory().GetAttachmentFromIndex(i);
					if (att && att.GetType() == attachClass)
					{
						matchingSourceAttachment = att;
						break;
					}
				}
				if (matchingSourceAttachment)
					CopyInventoryAttachmentsToPreview(matchingSourceAttachment, previewAttachment);
			}
		}
		
		// Aplicar magazines por último (ordem importa para preview)
		Weapon_Base weapon = Weapon_Base.Cast(preview);
		foreach (string magazineClass : magazines)
		{
			EntityAI magazineEntity = EntityAI.Cast(preview.GetInventory().CreateAttachment(magazineClass));
			if (!magazineEntity)
				continue;
			
			Magazine mag = Magazine.Cast(magazineEntity);
			if (mag)
			{
				// Preencher o magazine para forçar atualização do proxy visual
				int ammoMax = mag.GetAmmoMax();
				if (ammoMax > 0)
					mag.LocalSetAmmoCount(ammoMax);
				mag.Update();
			}
		}
		
		// Atualizar FSM e sincronizar estado da arma (se for uma arma)
		if (weapon)
		{
			weapon.RandomizeFSMState();
			weapon.Synchronize();
			weapon.ShowMagazine();
		}
	}
	
	protected int ComputeSellEstimate(EntityAI item)
	{
		if (!item)
			return 0;
		
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (!cache)
			return 0;
		
		AskalItemSyncData itemData = cache.FindItem(item.GetType());
		if (!itemData)
		{
			string normalized = NormalizeClassName(item.GetType());
			if (normalized != item.GetType())
				itemData = cache.FindItem(normalized);
		}
		if (!itemData)
			return 0;
		
		itemData.SellPercent = NormalizeSellPercent(itemData.SellPercent);
		itemData.BasePrice = NormalizeBuyPrice(itemData.BasePrice);
		
		// Cálculo simplificado no cliente (sem GetHealth01 que só funciona no servidor)
		// O servidor calculará o preço real considerando health, quantidade e attachments
		int basePrice = itemData.BasePrice;
		if (basePrice <= 0)
			basePrice = DEFAULT_HARDCODED_BUY_PRICE;
		
		int sellPercent = itemData.SellPercent;
		if (sellPercent <= 0)
			sellPercent = DEFAULT_HARDCODED_SELL_PERCENT;
		
		// Obter health do item (do servidor via RPC) ou usar 100% como fallback
		float healthPercent = GetItemHealth(item.GetType());
		
		// Estimativa base: preço base * percentual de venda * proporção de health
		// Preço proporcional à integridade (1% health = 1% do valor)
		float estimatedPrice = basePrice * (sellPercent / 100.0) * (healthPercent / 100.0);
		
		// Tenta estimar attachments de forma simplificada (sem recursão que usa GetHealth01)
		if (item.GetInventory())
		{
			int attachmentCount = item.GetInventory().AttachmentCount();
			for (int i = 0; i < attachmentCount; i++)
			{
				EntityAI attachment = item.GetInventory().GetAttachmentFromIndex(i);
				if (!attachment)
					continue;
				
				AskalItemSyncData attachData = cache.FindItem(attachment.GetType());
				if (attachData)
				{
					int attachBasePrice = attachData.BasePrice;
					if (attachBasePrice <= 0)
						attachBasePrice = DEFAULT_HARDCODED_BUY_PRICE;
					
					int attachSellPercent = attachData.SellPercent;
					if (attachSellPercent <= 0)
						attachSellPercent = DEFAULT_HARDCODED_SELL_PERCENT;
					
					// Obter health do attachment também
					float attachHealthPercent = GetItemHealth(attachment.GetType());
					
					// Preço do attachment proporcional à integridade
					estimatedPrice += attachBasePrice * (attachSellPercent / 100.0) * (attachHealthPercent / 100.0);
				}
			}
		}
		
		int totalPrice = Math.Round(estimatedPrice);
		if (totalPrice < 0)
			totalPrice = 0;
		
		return ApplySellCoefficient(totalPrice);
	}
	
	void ClearItems()
	{
		// Limpar widgets de itens
		if (m_ItemWidgets)
		{
			foreach (Widget widget : m_ItemWidgets)
			{
				if (widget)
					delete widget;
			}
			m_ItemWidgets.Clear();
		}
		
		if (m_ItemCardToIndex)
			m_ItemCardToIndex.Clear();
		if (m_ItemCardSelectionState)
			m_ItemCardSelectionState.Clear();
		if (m_ItemCardToInventoryItem)
			m_ItemCardToInventoryItem.Clear();
		
		// Limpar previews temporários
		if (m_PreviewItems)
		{
			foreach (EntityAI previewItem : m_PreviewItems)
			{
				if (previewItem)
					GetGame().ObjectDelete(previewItem);
			}
			m_PreviewItems.Clear();
		}
		
		// Limpar variantes
		ClearVariantsPanel();
		
		// Limpar attachments
		ClearAttachments();
		
		m_SelectedItemIndex = -1;
		m_SelectedVariantClass = "";
		m_SelectedItemUnitPrice = 0;
		if (m_ItemDatasetIds)
			m_ItemDatasetIds.Clear();
		if (m_ItemCategoryIds)
			m_ItemCategoryIds.Clear();
		m_CurrentSelectedClassName = "";
		m_CurrentItemMode = 0;
		m_CurrentCanBuy = false;
		m_CurrentCanSell = false;
		m_CurrentActionLayout = 0;
		m_SelectedInventoryItem = null;
		UpdateTransactionSummary();
	}
	
	// ========================================
	// SISTEMA DE VARIANTES
	// ========================================
	
	void RenderItemVariants(AskalItemData baseItemData)
	{
		if (!baseItemData)
		{
				HideVariantsPanel();
			return;
		}
		
		array<string> variants = baseItemData.GetVariants();
		int variantCount = 0;
		if (variants)
			variantCount = variants.Count();
		Print("[AskalStore] RenderItemVariants() - Base: " + baseItemData.GetClassName() + " | Variantes: " + variantCount);
		
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (!cache)
		{
			Print("[AskalStore] ⚠️ Cache não disponível em RenderItemVariants");
		}
		
		if (!m_VariantsScroll || !m_VariantsCardHolder)
		{
			Print("[AskalStore] ❌ VariantsScroll ou VariantsCardHolder é NULL!");
			return;
		}
		
		ClearVariantsPanel();
		
		m_VariantsScroll.Show(true);
		m_VariantsCardHolder.Show(true);
		m_CurrentVariantBaseClass = baseItemData.GetClassName();
		
		// Card do item base
		array<string> baseAttachments = baseItemData.GetDefaultAttachments();
		CreateVariantCardWidget(baseItemData.GetClassName(), baseAttachments, cache);
		
		// Cards das variantes
		if (variants && variants.Count() > 0)
		{
			foreach (string variantClass : variants)
			{
				if (!variantClass || variantClass == "" || variantClass == m_CurrentVariantBaseClass)
					continue;
				AskalItemSyncData variantSyncData = NULL;
				if (cache)
				{
					variantSyncData = cache.FindItem(variantClass);
				}
				array<string> variantAttachments = BuildAttachmentList(variantSyncData, cache, variantClass);
				CreateVariantCardWidget(variantClass, variantAttachments, cache);
			}
		}
		
		// FIX UpdateOffset
		if (m_VariantsCardHolder)
			m_VariantsCardHolder.Update();
		if (m_VariantsScroll)
			m_VariantsScroll.Update();
		
		if (m_VariantWidgets)
			Print("[AskalStore] ✅ Variantes renderizadas: " + m_VariantWidgets.Count());
		else
			Print("[AskalStore] ⚠️ m_VariantWidgets é NULL!");
	}

	protected void CreateVariantCardWidget(string className, array<string> attachments, AskalDatabaseClientCache cache)
	{
		Widget variantCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_variant_card.layout", m_VariantsCardHolder);
		if (!variantCard)
			return;
		
		ItemPreviewWidget variantPreview = ItemPreviewWidget.Cast(variantCard.FindAnyWidget("variant_preview_widget"));
		ButtonWidget variantButton = ButtonWidget.Cast(variantCard.FindAnyWidget("variant_card_button"));
		
				if (variantPreview)
				{
			EntityAI variantItem = EntityAI.Cast(SpawnTemporaryObject(className));
					if (variantItem)
					{
				array<string> resolvedAttachments = attachments;
				if (!resolvedAttachments)
				{
					AskalItemSyncData syncData = NULL;
					if (cache)
						syncData = cache.FindItem(className);
					resolvedAttachments = BuildAttachmentList(syncData, cache, className);
				}
				ApplyDefaultAttachmentsToEntity(variantItem, resolvedAttachments);
						variantPreview.SetItem(variantItem);
						variantPreview.SetModelPosition(Vector(0, 0, 0.5));
						variantPreview.SetModelOrientation(Vector(0, 0, 0));
						variantPreview.SetView(variantItem.GetViewIndex());
						variantPreview.Show(true);
						m_PreviewItems.Insert(variantItem);
			}
		}
		
				if (variantButton)
				{
			m_VariantCardToClassName.Set(variantButton, className);
		}
		
				m_VariantWidgets.Insert(variantCard);
	}
	
	void ClearVariantsPanel()
	{
		if (m_VariantWidgets)
		{
			foreach (Widget variantWidget : m_VariantWidgets)
			{
				if (variantWidget)
					delete variantWidget;
			}
			m_VariantWidgets.Clear();
		}
		
		if (m_VariantCardToClassName)
			m_VariantCardToClassName.Clear();
		m_CurrentVariantBaseClass = "";
	}
	
	void HideVariantsPanel()
	{
		if (m_VariantsScroll)
			m_VariantsScroll.Show(false);
		ClearVariantsPanel();
	}
	
	// ========================================
	// SISTEMA DE ATTACHMENTS
	// ========================================
	
	array<string> GetItemAttachmentSlotsFromConfig(string className)
	{
		array<string> slots = new array<string>();
		
		TStringArray configs = new TStringArray();
		configs.Insert("CfgVehicles");
		configs.Insert("CfgWeapons");
		configs.Insert("CfgMagazines");
		
		TStringArray foundSlots = new TStringArray();
		foreach (string configPath : configs)
		{
			string fullPath = configPath + " " + className + " attachments";
			GetGame().ConfigGetTextArray(fullPath, foundSlots);
			
			if (foundSlots.Count() > 0)
			{
				for (int i = 0; i < foundSlots.Count(); i++)
				{
					slots.Insert(foundSlots.Get(i));
				}
				
				if (configPath == "CfgWeapons")
					slots.Insert("magazine");
				
					break;
				}
			}
		
		return slots;
		}
		
	void RenderItemAttachments(AskalItemData itemData)
		{
		if (!m_AttachmentsCardHolder)
		{
			Print("[AskalStore] ⚠️ AttachmentsCardHolder não encontrado!");
			return;
		}
		
		ClearAttachments();
		
		if (!itemData)
		{
			m_AttachmentsCardHolder.Show(false);
			return;
		}
		
		array<string> attachments = itemData.GetDefaultAttachments();
		if (!attachments || attachments.Count() == 0)
		{
			m_AttachmentsCardHolder.Show(false);
			return;
		}
		
		m_AttachmentsCardHolder.Show(true);
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		
		foreach (string attachmentClass : attachments)
		{
			Widget attachCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_attachment_card.layout", m_AttachmentsCardHolder);
			if (!attachCard)
			{
				Print("[AskalStore] ❌ Falha ao criar attachment card para: " + attachmentClass);
				continue;
			}
			
			ImageWidget icon = ImageWidget.Cast(attachCard.FindAnyWidget("attachment_slot_icon"));
			if (icon)
			{
				icon.Show(false);
			}
			else
			{
				Print("[AskalStore] ⚠️ attachment_slot_icon não encontrado para: " + attachmentClass);
			}
			
			// O attachment_preview está dentro de attachment_card_button
			Widget attachButton = attachCard.FindAnyWidget("attachment_card_button");
			ItemPreviewWidget preview = NULL;
			if (attachButton)
			{
				preview = ItemPreviewWidget.Cast(attachButton.FindAnyWidget("attachment_preview"));
			}
			
			if (!preview)
			{
				preview = ItemPreviewWidget.Cast(attachCard.FindAnyWidget("attachment_preview"));
			}
			
			if (preview)
			{
				EntityAI attachmentEntity = EntityAI.Cast(SpawnTemporaryObject(attachmentClass));
				if (attachmentEntity)
				{
					preview.SetItem(attachmentEntity);
						preview.SetModelPosition(Vector(0, 0, 0.5));
					preview.SetView(attachmentEntity.GetViewIndex());
						preview.Show(true);
					m_PreviewItems.Insert(attachmentEntity);
					Print("[AskalStore] ✅ Attachment preview criado: " + attachmentClass);
				}
				else
				{
					preview.Show(false);
					Print("[AskalStore] ❌ Falha ao criar entidade para attachment: " + attachmentClass);
					}
				}
				else
				{
				Print("[AskalStore] ❌ attachment_preview não encontrado para: " + attachmentClass);
			}
			
			m_AttachmentCardToSlot.Set(attachCard, ResolveItemDisplayName(attachmentClass));
				m_AttachmentWidgets.Insert(attachCard);
		}
		
		m_AttachmentsCardHolder.Update();
	}
	
	void ClearAttachments()
	{
		if (m_AttachmentWidgets)
	{
		foreach (Widget attachWidget : m_AttachmentWidgets)
		{
			if (attachWidget)
				delete attachWidget;
		}
		m_AttachmentWidgets.Clear();
		}
		
		if (m_AttachmentCardToSlot)
			m_AttachmentCardToSlot.Clear();
	}
	
	protected void RenderAttachmentsFromInventory(EntityAI item)
	{
		if (!m_AttachmentsCardHolder)
			return;
		
		ClearAttachments();
		
		if (!item || !item.GetInventory())
		{
			m_AttachmentsCardHolder.Show(false);
			return;
		}
		
		int attachmentCount = item.GetInventory().AttachmentCount();
		if (attachmentCount == 0)
		{
			m_AttachmentsCardHolder.Show(false);
			return;
		}
		
		m_AttachmentsCardHolder.Show(true);
		
		for (int idx = 0; idx < attachmentCount; idx++)
		{
			EntityAI attachment = item.GetInventory().GetAttachmentFromIndex(idx);
			if (!attachment)
				continue;
			
			string attachmentClass = attachment.GetType();
			Widget attachCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_attachment_card.layout", m_AttachmentsCardHolder);
			if (!attachCard)
				continue;
			
			ItemPreviewWidget preview = ItemPreviewWidget.Cast(attachCard.FindAnyWidget("attachment_preview"));
			if (!preview)
			{
				Widget attachButton = attachCard.FindAnyWidget("attachment_card_button");
				if (attachButton)
					preview = ItemPreviewWidget.Cast(attachButton.FindAnyWidget("attachment_preview"));
			}
			
			if (preview)
			{
				EntityAI previewAttachment = EntityAI.Cast(SpawnTemporaryObject(attachmentClass));
				if (previewAttachment)
				{
					CopyInventoryAttachmentsToPreview(attachment, previewAttachment);
					preview.SetItem(previewAttachment);
					preview.SetModelPosition(Vector(0, 0, 0.5));
					preview.SetView(previewAttachment.GetViewIndex());
					preview.Show(true);
					m_PreviewItems.Insert(previewAttachment);
				}
			}
			
			TextWidget slotLabel = TextWidget.Cast(attachCard.FindAnyWidget("attachment_slot_label"));
			if (slotLabel)
				slotLabel.SetText(ResolveItemDisplayName(attachmentClass, ""));
			
			if (m_AttachmentCardToSlot)
				m_AttachmentCardToSlot.Set(attachCard, ResolveItemDisplayName(attachmentClass, ""));
			if (m_AttachmentWidgets)
				m_AttachmentWidgets.Insert(attachCard);
		}
		
		m_AttachmentsCardHolder.Update();
	}
	
	// ========================================
	// PREVIEW 3D PRINCIPAL
	// ========================================
	
	void UpdateSelectedItemPreview(EntityAI item)
	{
		if (!m_SelectedItemPreview)
			return;
		
		// Limpar cache anterior
		m_SelectedItemPreview.SetItem(NULL);
		
		// Aguardar 50ms para evitar race condition
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ForceSelectedPreviewRefresh, 50, false, item);
	}
	
	void ForceSelectedPreviewRefresh(EntityAI item)
	{
		if (!m_SelectedItemPreview || !item)
			return;
		
		m_SelectedItemPreview.SetItem(item);
		m_SelectedItemPreview.SetModelPosition(Vector(0, 0, 0.5));
		m_SelectedItemPreview.SetView(item.GetViewIndex());
		m_SelectedItemPreview.Show(true);
		
		// Resetar rotação e posição
		m_PreviewRotation = 0.0;
		m_PreviewPosition = Vector(0, 0, 0.5);
		m_PreviewZoom = 0.5;
		
		Print("[AskalStore] ✅ Preview principal atualizado!");
	}
	
	// ========================================
	// UPDATE PAINEL DIREITO
	// ========================================
	
	void UpdateRightPanel(AskalItemData itemData)
	{
		Print("[AskalStore] UpdateRightPanel() - Item: " + itemData.GetClassName());
		
		if (!m_ItemDetailsHolder)
		{
			Print("[AskalStore] ❌ ItemDetailsHolder é NULL!");
			return;
		}
		
		// Atualizar nome
		if (m_SelectedItemName)
		{
			string displayName = itemData.GetDisplayName();
			if (displayName == "")
				displayName = itemData.GetClassName();
			m_SelectedItemName.SetText(displayName);
		}
		
		// Atualizar preço unitário e resetar quantidade
		m_SelectedItemUnitPrice = itemData.GetPrice();
		ResetTransactionQuantity();
		
		// Atualizar preview 3D
		if (m_SelectedItemPreview)
		{
			EntityAI itemEntity = EntityAI.Cast(SpawnTemporaryObject(itemData.GetClassName()));
			if (itemEntity)
			{
				ApplyDefaultAttachmentsToEntity(itemEntity, itemData.GetDefaultAttachments());
				UpdateSelectedItemPreview(itemEntity);
				m_PreviewItems.Insert(itemEntity);
			}
		}
		
		// Renderizar variantes
		array<string> variants = itemData.GetVariants();
		if (variants && variants.Count() > 0)
		{
			RenderItemVariants(itemData);
		}
		else
		{
			HideVariantsPanel();
		}
		
		// Renderizar attachments
		RenderItemAttachments(itemData);
		UpdateTransactionSummary();
		
		Print("[AskalStore] ✅ Painel direito atualizado!");
	}
	
	void RefreshTransactionQuantityDisplay()
	{
		if (m_TransactionQuantityInput)
			m_TransactionQuantityInput.SetText(m_TransactionQuantity.ToString());
	}

	void DisplayTransactionMessage(string message)
	{
		DisplayTransactionMessageColored(message, ARGB(255, 255, 255, 255));
	}
	
	void DisplayTransactionMessageColored(string message, int color)
	{
		if (!m_TransactionDescription)
			return;
		
		m_TransactionDescription.SetText(message);
		m_TransactionDescription.SetColor(color);
	}
	
	void DisplayTransactionError(string message)
	{
		DisplayTransactionMessageColored(message, ARGB(255, 255, 64, 64));
	}

	protected string ResolveCurrencyShortName(string currencyId)
	{
		if (!currencyId || currencyId == "")
		{
			// Fallback to default
			AskalMarketConfig config = AskalMarketConfig.GetInstance();
			if (config)
				currencyId = config.GetDefaultCurrencyId();
			if (!currencyId || currencyId == "")
				return "???";
		}
		
		AskalMarketConfig config = AskalMarketConfig.GetInstance();
		if (!config)
			return "???";
		
		AskalCurrencyConfig currencyCfg = config.GetCurrencyConfig(currencyId);
		if (!currencyCfg && config.Currencies && config.Currencies.Contains(currencyId))
			currencyCfg = config.Currencies.Get(currencyId);
		
		if (currencyCfg && currencyCfg.ShortName != "")
			return currencyCfg.ShortName;
		
		// Fallback
		return "???";
	}
	
	protected void SetCurrencyShortNameText(TextWidget widget, string shortName)
	{
		if (!widget)
			return;
		
		if (!shortName || shortName == "")
		{
			widget.SetText("");
			widget.Show(false);
					}
					else
					{
			widget.SetText(shortName);
			widget.Show(true);
		}
	}
	
	protected void RefreshCurrencyShortname()
	{
		m_CurrentCurrencyShortName = ResolveCurrencyShortName(m_ActiveCurrencyId);
		SetCurrencyShortNameText(m_BuyCurrencyShortDual, m_CurrentCurrencyShortName);
		SetCurrencyShortNameText(m_SellCurrencyShortDual, m_CurrentCurrencyShortName);
		SetCurrencyShortNameText(m_BuyCurrencyShortWide, m_CurrentCurrencyShortName);
		SetCurrencyShortNameText(m_SellCurrencyShortWide, m_CurrentCurrencyShortName);
	}
	
	protected string FormatCurrencyValue(int amount)
	{
		bool negative = amount < 0;
		int absAmount = Math.AbsInt(amount);
		string digits = string.Format("%1", absAmount);
		string formatted = "";
		int groupCount = 0;
		
		for (int i = digits.Length() - 1; i >= 0; i--)
		{
			formatted = digits.Substring(i, 1) + formatted;
			groupCount++;
			
			if (groupCount == 3 && i > 0)
			{
				formatted = "." + formatted;
				groupCount = 0;
			}
		}
		
		if (formatted == "")
			formatted = "0";
		
		if (negative)
			formatted = "-" + formatted;
		
		return formatted + ",00";
	}
	
	protected int NormalizeBuyPrice(int price, int fallback = DEFAULT_HARDCODED_BUY_PRICE)
	{
		if (price <= 0)
			price = fallback;
		if (price <= 0)
			price = DEFAULT_HARDCODED_BUY_PRICE;
		return price;
	}
	
	protected int NormalizeSellPercent(int sellPercent, int fallback = DEFAULT_HARDCODED_SELL_PERCENT)
	{
		if (sellPercent <= 0)
			sellPercent = fallback;
		if (sellPercent <= 0)
			sellPercent = DEFAULT_HARDCODED_SELL_PERCENT;
		return sellPercent;
	}
	
	protected int ApplyBuyCoefficient(int price)
	{
		price = NormalizeBuyPrice(price);
		float adjustedFloat = price * m_BuyCoefficient;
		int adjusted = Math.Round(adjustedFloat);
		if (adjusted <= 0)
			adjusted = 1;
		return adjusted;
	}
	
	protected int ApplySellCoefficient(int price)
	{
		if (price <= 0)
			return 0;
		float adjustedFloat = price * m_SellCoefficient;
		int adjusted = Math.Round(adjustedFloat);
		if (m_SellCoefficient > 0 && adjusted <= 0)
			adjusted = 1;
		if (adjusted < 0)
			adjusted = 0;
		return adjusted;
	}
	
	protected void UpdateTotalText(MultilineTextWidget widget, int amount)
	{
		if (!widget)
			return;
		widget.SetText(FormatCurrencyValue(amount));
	}
	
	protected void ResetButtonTotals()
	{
		UpdateTotalText(m_BuyTotalTextDual, 0);
		UpdateTotalText(m_SellTotalTextDual, 0);
		UpdateTotalText(m_BuyTotalTextWide, 0);
		UpdateTotalText(m_SellTotalTextWide, 0);
	}
	
	protected void UpdateBuyTotalForLayout(int amount)
	{
		switch (m_CurrentActionLayout)
		{
			case 3:
				UpdateTotalText(m_BuyTotalTextDual, amount);
				break;
			case 1:
				UpdateTotalText(m_BuyTotalTextWide, amount);
				break;
			default:
				UpdateTotalText(m_BuyTotalTextDual, amount);
				UpdateTotalText(m_BuyTotalTextWide, amount);
				break;
		}
	}
	
	protected void UpdateSellTotalForLayout(int amount)
	{
		switch (m_CurrentActionLayout)
		{
			case 3:
				UpdateTotalText(m_SellTotalTextDual, amount);
				break;
			case 2:
				UpdateTotalText(m_SellTotalTextWide, amount);
				break;
			default:
				UpdateTotalText(m_SellTotalTextDual, amount);
				UpdateTotalText(m_SellTotalTextWide, amount);
				break;
		}
	}
	
	void UpdateTransactionSummary()
	{
		ResetButtonTotals();
		
		if (m_BatchBuyEnabled || m_BatchSellEnabled)
		{
			UpdateBatchTransactionSummary();
			return;
		}
		
		if (!m_TransactionDescription)
			return;
		
		if (m_SelectedItemIndex >= 0 && m_SelectedItemIndex < m_Items.Count())
		{
			AskalItemData itemData = m_Items.Get(m_SelectedItemIndex);
			if (itemData)
			{
				string className = itemData.GetClassName();
				if (m_SelectedVariantClass && m_SelectedVariantClass != "")
					className = m_SelectedVariantClass;
				
				int adjustedUnitPrice = CalculateAdjustedPrice(m_SelectedItemUnitPrice, className);
				int finalPrice = adjustedUnitPrice * m_TransactionQuantity;
				
				if (m_CurrentCanBuy)
					UpdateBuyTotalForLayout(finalPrice);
				
				int estimatedSell = 0;
				if (m_CurrentCanSell)
				{
					EntityAI inventoryItem = m_SelectedInventoryItem;
					if (!inventoryItem && className && className != "")
						inventoryItem = GetFirstItemInInventory(className);
					if (inventoryItem)
						estimatedSell = ComputeSellEstimate(inventoryItem);
				}
				UpdateSellTotalForLayout(estimatedSell);
				
				string breakdown = BuildPriceBreakdown(itemData);
				DisplayTransactionMessage(breakdown);
				return;
			}
		}
		
		if (m_SelectedInventoryItem)
		{
			int estimate = ComputeSellEstimate(m_SelectedInventoryItem);
			if (m_CurrentCanSell)
				UpdateSellTotalForLayout(estimate);
			DisplayTransactionMessage("Selecione itens do inventário para vender");
			return;
		}
		
		DisplayTransactionMessage("Selecione um item para ver os detalhes da transação");
	}
	
protected string BuildPriceBreakdown(AskalItemData itemData)
{
	if (!itemData)
		return "Selecione um item para ver os detalhes da transação";
	
	string effectiveClassName = m_SelectedVariantClass;
	if (!effectiveClassName || effectiveClassName == "")
		effectiveClassName = itemData.GetClassName();
	
	string displayName = ResolveItemDisplayName(effectiveClassName, itemData.GetDisplayName());
	if (!displayName || displayName == "")
		displayName = effectiveClassName;
	
	string breakdown = displayName;
	if (m_TransactionQuantity > 1)
		breakdown += " x" + m_TransactionQuantity.ToString();
	
	array<string> attachments = itemData.GetDefaultAttachments();
	string attachmentsSummary = "";
	if (attachments)
	{
		for (int i = 0; i < attachments.Count(); i++)
		{
			string attachmentClass = attachments.Get(i);
			if (!attachmentClass || attachmentClass == "")
				continue;
			
			string attachmentName = ResolveItemDisplayName(attachmentClass, "");
			if (!attachmentName || attachmentName == "")
				attachmentName = attachmentClass;
			
			if (attachmentsSummary != "")
				attachmentsSummary += ", ";
			attachmentsSummary += attachmentName;
		}
	}
	
	if (attachmentsSummary != "")
		breakdown += " + [" + attachmentsSummary + "]";
	
	if (m_SliderQuantityType == AskalItemQuantityType.MAGAZINE && m_CurrentAmmoCount > 0)
	{
		string ammoName = GetAmmoDisplayName(m_CurrentSelectedContent);
		if (!ammoName || ammoName == "")
			ammoName = m_CurrentSelectedContent;
		if (ammoName && ammoName != "")
			breakdown += " + " + m_CurrentAmmoCount.ToString() + "x " + ammoName;
	}
	else if (m_SliderQuantityType == AskalItemQuantityType.STACKABLE && m_CurrentAmmoCount > 0)
	{
		breakdown += " + " + m_CurrentAmmoCount.ToString() + " unidades";
	}
	else if (m_SliderQuantityType == AskalItemQuantityType.QUANTIFIABLE && m_CurrentItemIsLiquidContainer)
	{
		string liquidName = "";
		if (m_AvailableLiquidTypes && m_CurrentContentIndex >= 0 && m_CurrentContentIndex < m_AvailableLiquidTypes.Count())
		{
			int liquidType = m_AvailableLiquidTypes.Get(m_CurrentContentIndex);
			liquidName = GetLiquidDisplayName(liquidType);
		}
		
		if (!liquidName || liquidName == "")
			liquidName = "Conteúdo";
		
		breakdown += " + " + m_CurrentQuantityPercent.ToString() + "% " + liquidName;
	}
	
	int baseUnitPrice = m_SelectedItemUnitPrice;
	if (baseUnitPrice <= 0)
		baseUnitPrice = itemData.GetPrice();
	if (baseUnitPrice <= 0)
		baseUnitPrice = DEFAULT_HARDCODED_BUY_PRICE;
	
	int adjustedUnitPrice = CalculateAdjustedPrice(baseUnitPrice, effectiveClassName);
	if (adjustedUnitPrice < 0)
		adjustedUnitPrice = 0;
	
	int totalQuantity = Math.Max(m_TransactionQuantity, 1);
	int totalPrice = adjustedUnitPrice * totalQuantity;
	
	string totalString = FormatCurrencyValue(totalPrice);
	if (m_CurrentCurrencyShortName && m_CurrentCurrencyShortName != "")
		totalString = totalString + " " + m_CurrentCurrencyShortName;
	
	breakdown += " = " + totalString;
	return breakdown;
}

	protected void UpdateBatchTransactionSummary()
	{
		if (!m_TransactionDescription)
			return;
		
		if (m_BatchBuyEnabled)
		{
			int selectedCount = 0;
			int totalPrice = 0;
			if (m_BatchBuySelectedIndexes)
			{
				for (int idx = 0; idx < m_BatchBuySelectedIndexes.Count(); idx++)
				{
					int itemIdx = m_BatchBuySelectedIndexes.GetKey(idx);
					if (itemIdx >= 0 && itemIdx < m_Items.Count())
					{
						AskalItemData data = m_Items.Get(itemIdx);
						if (data)
						{
							selectedCount++;
							totalPrice += data.GetPrice();
						}
					}
				}
			}
			
			string summaryBuy = "Itens selecionados: " + selectedCount;
			UpdateBuyTotalForLayout(totalPrice);
			DisplayTransactionMessage(summaryBuy);
			return;
		}
		
		if (m_BatchSellEnabled)
		{
			int selectedSellCount = 0;
			int estimatedGain = 0;
			if (m_BatchSellSelectedEntities)
			{
				foreach (EntityAI selectedItem : m_BatchSellSelectedEntities)
				{
					if (!selectedItem)
						continue;
					selectedSellCount++;
					estimatedGain += ComputeSellEstimate(selectedItem);
				}
			}
			
			string summarySell = "Itens selecionados: " + selectedSellCount;
			UpdateSellTotalForLayout(estimatedGain);
			DisplayTransactionMessage(summarySell);
			return;
		}
		
		DisplayTransactionMessage("Selecione itens para iniciar a venda em lote");
	}
	
	void SetTransactionQuantity(int quantity)
	{
		if (quantity < 1)
			quantity = 1;
		if (quantity > 9999)
			quantity = 9999;
		
		m_TransactionQuantity = quantity;
		RefreshTransactionQuantityDisplay();
		UpdateTransactionSummary();
	}

	void IncrementTransactionQuantity(int delta)
	{
		SetTransactionQuantity(m_TransactionQuantity + delta);
	}

	void ResetTransactionQuantity()
	{
		SetTransactionQuantity(1);
	}

	void UpdateBreadcrumb(string datasetDisplayName, string categoryDisplayName)
	{
		if (!m_ItemsBreadcrumbText)
			return;
		
		string breadcrumbText = categoryDisplayName;
		if (datasetDisplayName != "")
			breadcrumbText = datasetDisplayName + " > " + categoryDisplayName;
		
		m_ItemsBreadcrumbText.SetText(breadcrumbText);
	}
	
	// ========================================
	// HANDLERS DE CLICK
	// ========================================
	
	bool IsWidgetChildOf(Widget child, Widget potentialParent, int maxDepth = 5)
	{
		if (!child || !potentialParent)
			return false;
		
		Widget current = child;
		int depth = 0;
		while (current && depth <= maxDepth)
		{
			if (current == potentialParent)
				return true;
			current = current.GetParent();
			depth++;
		}
		
		return false;
	}
	
	bool FindDatasetCategoryForWidget(Widget target, out int datasetIdx, out int categoryIdx)
	{
		datasetIdx = -1;
		categoryIdx = -1;
		if (!target)
			return false;
		
		for (int d = 0; d < m_DatasetCategoryButtonsPerDataset.Count(); d++)
		{
			array<ButtonWidget> buttonArray = m_DatasetCategoryButtonsPerDataset.Get(d);
			array<Widget> rootArray = m_DatasetCategoryCardRootsPerDataset.Get(d);
			for (int c = 0; c < buttonArray.Count(); c++)
			{
			ButtonWidget categoryButton = buttonArray.Get(c);
			Widget categoryRoot = NULL;
			if (c < rootArray.Count())
			{
				categoryRoot = rootArray.Get(c);
			}
			
			bool matched = false;
			if (categoryButton)
			{
				if (target == categoryButton || IsWidgetChildOf(target, categoryButton, 6))
				{
					matched = true;
				}
			}
			if (!matched && categoryRoot)
			{
				if (IsWidgetChildOf(target, categoryRoot, 6))
				{
					matched = true;
				}
			}
			
			if (matched)
			{
				datasetIdx = d;
				categoryIdx = c;
				return true;
			}
			}
		}
		
		return false;
	}

	// ========================================
	// MODO LOTE (COMPRA / VENDA)
	// ========================================
	
	protected void ToggleBatchBuyMode()
	{
		bool newState = !m_BatchBuyEnabled;
		if (newState && m_BatchSellEnabled)
			SetBatchSellEnabled(false);
		SetBatchBuyEnabled(newState);
	}
	
	protected void ToggleBatchSellMode()
	{
		bool newState = !m_BatchSellEnabled;
		if (newState && m_BatchBuyEnabled)
			SetBatchBuyEnabled(false);
		SetBatchSellEnabled(newState);
	}
	
	protected void SetBatchBuyEnabled(bool enabled)
	{
		if (m_BatchBuyEnabled == enabled)
			return;
		
		m_BatchBuyEnabled = enabled;
		UpdateBatchToggleVisual(m_BatchBuyToggle, enabled);
		ClearBatchSelections(true);
		
		if (!enabled)
		{
			UpdateTransactionSummary();
			DisplayTransactionMessage("Selecione um item para ver os detalhes da transação");
			RefreshActionButtonsForSelection();
		}
		else
		{
			// Ao ativar compra em lote, garantimos que estamos visualizando o catálogo atual
			if (m_ShowingInventoryForSale)
			{
				m_ShowingInventoryForSale = false;
				LoadCategory(m_CurrentCategoryIndex);
			}
			UpdateActionButtonsForBatch(true);
			UpdateTransactionSummary();
			DisplayTransactionMessage("Selecione itens do catálogo para comprar em lote");
		}
	}
	
	protected void SetBatchSellEnabled(bool enabled)
	{
		if (m_BatchSellEnabled == enabled)
			return;
		
		m_BatchSellEnabled = enabled;
		UpdateBatchToggleVisual(m_BatchSellToggle, enabled);
		ClearBatchSelections(true);
		
		if (enabled)
		{
			ScanPlayerInventory();
			ShowPlayerInventoryCard(true);
			m_ShowingInventoryForSale = true;
			RenderInventoryItemsForSale();
			UpdateActionButtonsForBatch(false);
			UpdateTransactionSummary();
			DisplayTransactionMessage("Selecione itens do inventário para vender em lote");
		}
		else
		{
			ShowPlayerInventoryCard(false);
			m_ShowingInventoryForSale = false;
			LoadCategory(m_CurrentCategoryIndex);
			UpdateTransactionSummary();
			DisplayTransactionMessage("Selecione um item para ver os detalhes da transação");
			RefreshActionButtonsForSelection();
		}
	}
	
	protected void UpdateBatchToggleVisual(ButtonWidget toggle, bool enabled)
	{
		if (!toggle)
			return;
		
		if (enabled)
			toggle.SetColor(ARGB(200, 255, 0, 0));
		else
			toggle.SetColor(ARGB(150, 0, 0, 0));
	}
	
	protected void ShowPlayerInventoryCard(bool show)
	{
		if (m_PlayerInventoryCard)
			m_PlayerInventoryCard.Show(show);
	}
	
	protected void ClearBatchSelections(bool clearVisuals)
	{
		if (clearVisuals && m_ItemCardSelectionState)
		{
			array<Widget> cardsToReset = new array<Widget>();
			for (int idx = 0; idx < m_ItemCardSelectionState.Count(); idx++)
			{
				Widget card = m_ItemCardSelectionState.GetKey(idx);
				if (card)
					cardsToReset.Insert(card);
			}
			
			foreach (Widget resetCard : cardsToReset)
			{
				SetCardSelected(resetCard, false);
			}
			
			m_ItemCardSelectionState.Clear();
		}
		
		if (m_BatchBuySelectedIndexes)
			m_BatchBuySelectedIndexes.Clear();
		if (m_BatchSellSelectedEntities)
			m_BatchSellSelectedEntities.Clear();
	}
	
	protected void SetCardSelected(Widget card, bool selected)
	{
		if (!card)
			return;
		
		ImageWidget selectedIcon = ImageWidget.Cast(card.FindAnyWidget("item_selected_check"));
		if (selectedIcon)
			selectedIcon.Show(selected);
		
		if (!m_ItemCardSelectionState)
			return;
		
		if (selected)
			m_ItemCardSelectionState.Set(card, true);
		else if (m_ItemCardSelectionState.Contains(card))
			m_ItemCardSelectionState.Remove(card);
	}
	
	protected bool IsCardSelected(Widget card)
	{
		if (!card || !m_ItemCardSelectionState)
			return false;
		return m_ItemCardSelectionState.Contains(card);
	}
	
	protected void OnPlayerInventoryCardSelected()
	{
		if (!m_BatchSellEnabled)
			return;
		
		m_ShowingInventoryForSale = true;
		ScanPlayerInventory();
		RenderInventoryItemsForSale();
	}
	
	protected void HandleBatchBuySelection(Widget itemCard, int itemIndex)
	{
		if (!m_BatchBuyEnabled || itemIndex < 0 || itemIndex >= m_Items.Count())
			return;
		
		bool alreadySelected = m_BatchBuySelectedIndexes && m_BatchBuySelectedIndexes.Contains(itemIndex);
		if (alreadySelected)
		{
			m_BatchBuySelectedIndexes.Remove(itemIndex);
			SetCardSelected(itemCard, false);
		}
		else
		{
			m_BatchBuySelectedIndexes.Set(itemIndex, true);
			SetCardSelected(itemCard, true);
		}
		
		m_SelectedItemIndex = itemIndex;
		m_SelectedInventoryItem = null;
		AskalItemData itemData = m_Items.Get(itemIndex);
		if (itemData)
		{
			m_SelectedItemUnitPrice = itemData.GetPrice();
			UpdateRightPanel(itemData);
			ConfigureQuantitySliderForItem(itemData.GetClassName());
		}
		
		UpdateTransactionSummary();
	}
	
	protected void HandleBatchSellSelection(Widget itemCard, int infoIndex)
	{
		if (!m_BatchSellEnabled)
			return;
		
		AskalInventoryDisplayInfo info = GetInventoryDisplayInfo(infoIndex);
		
		EntityAI inventoryItem = NULL;
		if (info && info.Item)
		{
			inventoryItem = info.Item;
		}
		else if (m_ItemCardToInventoryItem && m_ItemCardToInventoryItem.Contains(itemCard))
		{
			inventoryItem = m_ItemCardToInventoryItem.Get(itemCard);
		}
		
		if (!inventoryItem)
			return;
		
		int existingIdx = -1;
		if (m_BatchSellSelectedEntities)
			existingIdx = m_BatchSellSelectedEntities.Find(inventoryItem);
		
		if (existingIdx != -1)
		{
			m_BatchSellSelectedEntities.Remove(existingIdx);
			SetCardSelected(itemCard, false);
		}
		else
		{
			m_BatchSellSelectedEntities.Insert(inventoryItem);
			SetCardSelected(itemCard, true);
		}
		
		m_SelectedInventoryItem = inventoryItem;
		m_SelectedItemIndex = infoIndex;
		
		if (info)
		{
			m_SelectedItemUnitPrice = info.EstimatedPrice;
			UpdateRightPanelFromInventory(info);
		}
		else
		{
			m_SelectedItemUnitPrice = ComputeSellEstimate(inventoryItem);
			AskalInventoryDisplayInfo tempInfo = new AskalInventoryDisplayInfo();
			tempInfo.Item = inventoryItem;
			tempInfo.ClassName = inventoryItem.GetType();
			ItemBase tempBase = ItemBase.Cast(inventoryItem);
			string fallbackDisplay = "";
			if (tempBase)
				fallbackDisplay = tempBase.GetDisplayName();
			tempInfo.DisplayName = ResolveItemDisplayName(tempInfo.ClassName, fallbackDisplay);
			tempInfo.EstimatedPrice = m_SelectedItemUnitPrice;
			// Obter health do servidor (via RPC) ou usar 100% como fallback
			tempInfo.HealthPercent = GetItemHealth(tempInfo.ClassName);
			UpdateRightPanelFromInventory(tempInfo);
		}
		
		UpdateTransactionSummary();
	}
	
	protected AskalInventoryDisplayInfo GetInventoryDisplayInfo(int index)
	{
		if (!m_InventoryDisplayItems || index < 0 || index >= m_InventoryDisplayItems.Count())
			return NULL;
		return m_InventoryDisplayItems.Get(index);
	}
	
	protected void UpdateRightPanelFromInventory(AskalInventoryDisplayInfo info)
	{
		if (!info || !info.Item)
			return;
		
		m_CurrentSelectedClassName = info.ClassName;
		m_SelectedInventoryItem = info.Item;
		
		if (m_SelectedItemName)
			m_SelectedItemName.SetText(info.DisplayName);
		
		m_SelectedItemUnitPrice = info.EstimatedPrice;
		SetTransactionQuantity(1);
		
		if (m_TransactionQuantitySlider)
			m_TransactionQuantitySlider.Show(false);
		if (m_TransactionContentPanel)
			m_TransactionContentPanel.Show(false);
		HideVariantsPanel();
		
		if (m_SelectedItemPreview)
		{
			EntityAI previewEntity = EntityAI.Cast(SpawnTemporaryObject(info.ClassName));
			if (previewEntity)
			{
				CopyInventoryAttachmentsToPreview(info.Item, previewEntity);
				UpdateSelectedItemPreview(previewEntity);
				m_PreviewItems.Insert(previewEntity);
			}
		}
		
		RenderAttachmentsFromInventory(info.Item);
		
		// IMPORTANTE: Atualizar botões baseado no modo do item do trader
		int itemMode = ResolveItemModeForClass(info.ClassName, -1);
		UpdateActionButtons(itemMode);
		Print("[AskalStore] 🔘 Botões atualizados para item do inventário: " + info.ClassName + " (modo: " + itemMode + ")");
		
		UpdateTransactionSummary();
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_BuyButton && !m_SellButton)
		{
			Print("[AskalStore] ⚠️ ATENÇÃO: Botões Buy/Sell NÃO foram inicializados!");
		}
		
		// IMPORTANTE: Verificar botões PRIMEIRO, antes de qualquer outra coisa
		// Botões principais (Buy/Sell no painel direito) - VERIFICAR PRIMEIRO
		if (w == m_BuyButton)
		{
			Print("[AskalStore] 🛒 buy_button (painel direito) clicado");
			OnPurchaseClick(m_BuyButton);
			return true;
		}
		
		if (w == m_SellButton)
		{
			Print("[AskalStore] 💵 sell_button (painel dual) clicado");
			OnSellClick(m_SellButton);
			return true;
		}
		
		if (w == m_BuyButtonSolo)
		{
			Print("[AskalStore] 🛒 buy_button_solo (painel single) clicado");
			OnPurchaseClick(m_BuyButtonSolo);
			return true;
		}
		
		if (w == m_SellButtonSolo)
		{
			Print("[AskalStore] 💵 sell_button_solo (painel single) clicado");
			OnSellClick(m_SellButtonSolo);
			return true;
		}
		
		// Toggle batch buy
		if (m_BatchBuyToggle && IsWidgetChildOf(w, m_BatchBuyToggle, 3))
		{
			Print("[AskalStore] 🔄 toggle_batch_buy clicado");
			ToggleBatchBuyMode();
			return true;
		}
		
		// Toggle batch sell
		if (m_BatchSellToggle && IsWidgetChildOf(w, m_BatchSellToggle, 3))
		{
			Print("[AskalStore] 🔄 toggle_batch_sell clicado");
			ToggleBatchSellMode();
			return true;
		}
		
		// Card especial do inventário
		if (m_PlayerInventoryHeaderButton && IsWidgetChildOf(w, m_PlayerInventoryHeaderButton, 3))
		{
			Print("[AskalStore] 📦 player_Inventory_card clicado");
			OnPlayerInventoryCardSelected();
			return true;
		}
		
		// Verificar por nome também (caso widget seja um filho) - NOVOS NOMES
			if (w && (w.GetName() == "Buy_Button" || w.GetName() == "buy_small_button"))
			{
				Print("[AskalStore] 🛒 Buy_Button clicado (por nome: " + w.GetName() + ")!");
				OnPurchaseClick(GetActiveBuyButton());
				return true;
			}
			
			if (w && (w.GetName() == "Sell_Button" || w.GetName() == "sell_big_button"))
			{
				Print("[AskalStore] 💵 Sell_Button clicado (por nome: " + w.GetName() + ")!");
				OnSellClick(GetActiveSellButton());
				return true;
			}
			
			// Verificar parents até 5 níveis (para painéis/textos dentro dos botões)
			if (m_BuyButton)
			{
				Widget checkBuyWidget = w;
				int buyDepth = 0;
				while (checkBuyWidget && buyDepth < 5)
				{
					if (checkBuyWidget == m_BuyButton)
					{
						Print("[AskalStore] 🛒 buy_button clicado (via parent, depth: " + buyDepth + ")!");
						OnPurchaseClick(m_BuyButton);
						return true;
					}
					checkBuyWidget = checkBuyWidget.GetParent();
					buyDepth++;
				}
			}
			
			if (m_SellButton)
			{
				Widget checkSellWidget = w;
				int sellDepth = 0;
				while (checkSellWidget && sellDepth < 5)
				{
					if (checkSellWidget == m_SellButton)
					{
						Print("[AskalStore] 💵 sell_button clicado (via parent, depth: " + sellDepth + ")!");
						OnSellClick(m_SellButton);
						return true;
					}
					checkSellWidget = checkSellWidget.GetParent();
					sellDepth++;
				}
			}
			
			// Controle de quantidade com botões +/- (quando disponíveis)
			if (w == m_TransactionQuantityPlusButton)
			{
				IncrementTransactionQuantity(1);
				return true;
			}
			
			if (w == m_TransactionQuantityMinusButton)
			{
				IncrementTransactionQuantity(-1);
				return true;
			}
			
			// Navegação do seletor de conteúdo (munições/líquidos)
			if (w == m_TransactionContentPrevButton)
			{
				OnContentSelectorPrevClick();
				return true;
			}
			
			if (w == m_TransactionContentNextButton)
			{
				OnContentSelectorNextClick();
				return true;
			}
			
			super.OnClick(w, x, y, button);
			int depth = 0;
			Widget itemCard = null;
			AskalItemData itemData = null;
			
			// Fechar menu
			if (w == m_CloseButton)
			{
				Print("[AskalStore] Fechando menu...");
				Close();
				return true;
			}
			
			// Click em dataset card (botão do cabeçalho)
			for (int datasetClickIdx = 0; datasetClickIdx < m_DatasetButtons.Count(); datasetClickIdx++)
			{
				ButtonWidget datasetButton = m_DatasetButtons.Get(datasetClickIdx);
				if (!datasetButton)
					continue;
				
				if (w == datasetButton || IsWidgetChildOf(w, datasetButton, 4))
				{
					Print("[AskalStore] 📁 Dataset card clicado: índice " + datasetClickIdx);
					bool isExpanded = true;
					if (datasetClickIdx < m_DatasetExpandedStates.Count())
						isExpanded = m_DatasetExpandedStates.Get(datasetClickIdx);
					bool isCurrent = (datasetClickIdx == m_CurrentDatasetIndex);
					
					if (!isExpanded)
					{
						Print("[AskalStore] 📂 Expandindo dataset " + datasetClickIdx);
						SetDatasetExpanded(datasetClickIdx, true);
						LoadDataset(datasetClickIdx);
						if (m_Categories.Count() > 0)
							LoadCategory(0);
			}
		else
		{
						if (!isCurrent)
						{
							LoadDataset(datasetClickIdx);
							if (m_Categories.Count() > 0)
								LoadCategory(0);
		}
		else
		{
							Print("[AskalStore] 📁 Colapsando dataset atual");
							SetDatasetExpanded(datasetClickIdx, false);
						}
					}
					return true;
				}
			}
			
			// Controle de quantidade da transação
			if (w == m_TransactionQuantityPlusButton)
			{
				IncrementTransactionQuantity(1);
				return true;
			}
			if (w == m_TransactionQuantityMinusButton)
			{
				IncrementTransactionQuantity(-1);
				return true;
			}
			
			int datasetIdxFound;
			int categoryIdxFound;
			if (FindDatasetCategoryForWidget(w, datasetIdxFound, categoryIdxFound))
			{
				Print("[AskalStore] 📑 Category card clicado → Dataset " + datasetIdxFound + " | Categoria " + categoryIdxFound);
				if (datasetIdxFound >= 0 && datasetIdxFound < m_Datasets.Count())
				{
					if (datasetIdxFound != m_CurrentDatasetIndex)
					{
						LoadDataset(datasetIdxFound);
					}
					if (categoryIdxFound >= 0)
					{
						if (categoryIdxFound >= m_Categories.Count())
							categoryIdxFound = m_Categories.Count() - 1;
						if (categoryIdxFound >= 0)
							LoadCategory(categoryIdxFound);
					}
				}
				return true;
			}
			
			// Click em item card
			if (m_ItemCardToIndex.Contains(w))
			{
				itemCard = w;
				}
			else
			{
				// Procurar nos parents
				Widget current = w.GetParent();
				int parentDepth = 0;
				while (current && parentDepth < 5)
				{
					if (m_ItemCardToIndex.Contains(current))
					{
						itemCard = current;
							break;
						}
					current = current.GetParent();
					parentDepth++;
				}
			}
			
			if (itemCard)
			{
				int mappedIndex = -1;
				if (m_ItemCardToIndex && m_ItemCardToIndex.Contains(itemCard))
					mappedIndex = m_ItemCardToIndex.Get(itemCard);
				
				if (m_ShowingInventoryForSale)
				{
					HandleBatchSellSelection(itemCard, mappedIndex);
					return true;
				}
				
				if (m_BatchSellEnabled && m_ItemCardToInventoryItem && m_ItemCardToInventoryItem.Contains(itemCard))
				{
					HandleBatchSellSelection(itemCard, mappedIndex);
					return true;
				}
				
				if (m_BatchBuyEnabled)
				{
					HandleBatchBuySelection(itemCard, mappedIndex);
					return true;
				}
				
				if (mappedIndex >= 0 && mappedIndex < m_Items.Count())
				{
					m_SelectedItemIndex = mappedIndex;
					m_SelectedVariantClass = "";
					itemData = m_Items.Get(mappedIndex);
					
					if (m_ItemCardToInventoryItem && m_ItemCardToInventoryItem.Contains(itemCard))
						m_SelectedInventoryItem = m_ItemCardToInventoryItem.Get(itemCard);
					else
						m_SelectedInventoryItem = null;
					
					m_CurrentSelectedClassName = itemData.GetClassName();
					int resolvedMode = ResolveItemModeForClass(m_CurrentSelectedClassName, mappedIndex);
					UpdateActionButtons(resolvedMode);
					
					UpdateRightPanel(itemData);
					ConfigureQuantitySliderForItem(itemData.GetClassName());
					Print("[AskalStore] ✅ Item selecionado: " + itemData.GetClassName());
					return true;
				}
			}
			
			// Click em variante
			if (m_VariantCardToClassName.Contains(w))
			{
				string variantClass = m_VariantCardToClassName.Get(w);
				if (m_SelectedItemIndex >= 0 && m_SelectedItemIndex < m_Items.Count())
				{
					AskalItemData baseItem = m_Items.Get(m_SelectedItemIndex);
					if (variantClass == m_CurrentVariantBaseClass)
					{
						m_SelectedVariantClass = "";
						if (baseItem)
						{
							string baseDisplayName = baseItem.GetDisplayName();
							if (!baseDisplayName || baseDisplayName == "")
								baseDisplayName = ResolveItemDisplayName(baseItem.GetClassName(), "");
							if (m_SelectedItemName)
								m_SelectedItemName.SetText(baseDisplayName);
							if (m_SelectedItemPreview)
							{
								EntityAI baseEntity = EntityAI.Cast(SpawnTemporaryObject(baseItem.GetClassName()));
								if (baseEntity)
								{
									ApplyDefaultAttachmentsToEntity(baseEntity, baseItem.GetDefaultAttachments());
									UpdateSelectedItemPreview(baseEntity);
									m_PreviewItems.Insert(baseEntity);
								}
							}
							m_SelectedItemUnitPrice = baseItem.GetPrice();
							m_CurrentSelectedClassName = baseItem.GetClassName();
							int baseMode = ResolveItemModeForClass(m_CurrentSelectedClassName, m_SelectedItemIndex);
							UpdateActionButtons(baseMode);
							UpdateTransactionSummary();
							RenderItemAttachments(baseItem);
							Print("[AskalStore] ✅ Variante resetada para item base: " + baseItem.GetClassName());
						}
						return true;
					}
					
					m_SelectedVariantClass = variantClass;
					AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
					AskalItemSyncData variantData = NULL;
					if (cache)
					{
						variantData = cache.FindItem(variantClass);
					}
					array<string> variantAttachments = BuildAttachmentList(variantData, cache, variantClass);
					
					// Configurar slider de quantidade para a variante selecionada
					ConfigureQuantitySliderForItem(variantClass);
					
					// Atualizar preview com variante
					if (m_SelectedItemPreview)
					{
						EntityAI variantEntity = EntityAI.Cast(SpawnTemporaryObject(variantClass));
						if (variantEntity)
						{
							ApplyDefaultAttachmentsToEntity(variantEntity, variantAttachments);
							UpdateSelectedItemPreview(variantEntity);
							m_PreviewItems.Insert(variantEntity);
						}
					}
					
					// Atualizar nome/preço da variante
					string displayName = ResolveItemDisplayName(variantClass, "");
					if (variantData)
					{
						displayName = ResolveItemDisplayName(variantClass, variantData.DisplayName);
					}
					if (m_SelectedItemName)
						m_SelectedItemName.SetText(displayName);
					
					int variantBasePrice = 0;
					if (variantData)
					{
						variantBasePrice = variantData.BasePrice;
					}
					int variantFallbackPrice = DEFAULT_HARDCODED_BUY_PRICE;
					if (baseItem)
						variantFallbackPrice = baseItem.GetBasePrice();
					if (variantBasePrice <= 0 && baseItem)
						variantBasePrice = baseItem.GetBasePrice();
					variantBasePrice = NormalizeBuyPrice(variantBasePrice, variantFallbackPrice);
					
					int variantTotalPrice = ComputeTotalItemPrice(variantBasePrice, variantAttachments, cache);
					variantTotalPrice = NormalizeBuyPrice(variantTotalPrice, variantBasePrice);
					variantTotalPrice = ApplyBuyCoefficient(variantTotalPrice);
					if (variantTotalPrice <= 0 && baseItem)
						variantTotalPrice = ApplyBuyCoefficient(NormalizeBuyPrice(baseItem.GetPrice(), variantBasePrice));
					
					m_SelectedItemUnitPrice = variantTotalPrice;
					m_CurrentSelectedClassName = variantClass;
					int variantMode = ResolveItemModeForClass(variantClass, m_SelectedItemIndex);
					UpdateActionButtons(variantMode);
					UpdateTransactionSummary();
					
					AskalItemData tempVariantData = new AskalItemData();
					tempVariantData.SetClassName(variantClass);
					tempVariantData.SetDisplayName(displayName);
					tempVariantData.SetBasePrice(variantBasePrice);
					tempVariantData.SetPrice(variantTotalPrice);
					tempVariantData.SetDefaultAttachments(variantAttachments);
					RenderItemAttachments(tempVariantData);
					
					Print("[AskalStore] ✅ Variante selecionada: " + variantClass);
					return true;
				}
			}
			
			
			return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_TransactionQuantityInput)
		{
			string quantityText = m_TransactionQuantityInput.GetText();
			quantityText = quantityText.Trim();
			int parsedQuantity = quantityText.ToInt();
			SetTransactionQuantity(parsedQuantity);
			return true;
		}
		
		// Slider de quantidade (para magazines, stackables, etc)
		if (w == m_TransactionQuantitySlider)
		{
			if (finished) // Só processa quando o slider para de ser arrastado
			{
				float sliderValue = m_TransactionQuantitySlider.GetCurrent();
				OnQuantitySliderChanged(sliderValue);
			}
			return true;
		}
		
		return super.OnChange(w, x, y, finished);
	}
	
	// ========================================
	// COMPRA E VENDA
	// ========================================
	
	// ========================================
	// SISTEMA DE QUANTIDADE VARIÁVEL
	// ========================================
	
	/// Detecta o tipo de quantidade do item
	AskalItemQuantityType DetectItemQuantityType(string className)
	{
		// Cria uma instância temporária para detectar o tipo
		Object obj = SpawnTemporaryObject(className);
		if (!obj)
			return AskalItemQuantityType.NONE;
		
		AskalItemQuantityType result = AskalItemQuantityType.NONE;
		
		// Verifica se é um Magazine
		Magazine mag = Magazine.Cast(obj);
		if (mag)
		{
			result = AskalItemQuantityType.MAGAZINE;
			if (obj)
				GetGame().ObjectDelete(obj);
			return result;
		}
		
	// Verifica se é um item com quantidade
	ItemBase item = ItemBase.Cast(obj);
	if (item && item.HasQuantity())
	{
		// Verifica se tem range de quantidade válido
		if (item.GetQuantityMax() - item.GetQuantityMin() > 0)
		{
			// STACKABLE: Itens que podem ser divididos (split)
			// Inclui munição, pregos, tábuas, etc automaticamente
			if (item.IsSplitable())
			{
				result = AskalItemQuantityType.STACKABLE;
			}
			// QUANTIFIABLE: APENAS Bottle_Base (containers de líquido)
			// Outros fracionáveis (bandagens, carnes, etc) ficam como NONE (sempre 100%)
			else if (item.IsLiquidContainer())
			{
				result = AskalItemQuantityType.QUANTIFIABLE;
			}
			// Fracionáveis genéricos ficam como NONE (sem slider, sempre 100%)
			else
			{
				result = AskalItemQuantityType.NONE;
			}
		}
	}
		
		if (obj)
			GetGame().ObjectDelete(obj);
		return result;
	}
	
	/// Obtém os valores min/max para o slider baseado no tipo de item
	void GetQuantityRangeForItem(string className, out int minValue, out int maxValue, out string unit, out float stepValue)
	{
		minValue = 0;
		maxValue = 100;
		unit = "";
		stepValue = 1.0;
		
		AskalItemQuantityType qtyType = DetectItemQuantityType(className);
		if (qtyType == AskalItemQuantityType.NONE)
				return;
		
		Object tempObj = NULL;
		ItemBase item = NULL;
		Magazine mag = NULL;
		
		switch (qtyType)
		{
			case AskalItemQuantityType.MAGAZINE:
			{
				tempObj = SpawnTemporaryObject(className);
				mag = Magazine.Cast(tempObj);
				if (mag)
				{
				int ammoMax = mag.GetAmmoMax();
				if (ammoMax <= 0)
					ammoMax = 1;
				
				if (mag.IsAmmoPile() && ammoMax > 1)
					minValue = 1;
				else
					minValue = 0;
				
				maxValue = ammoMax;
				stepValue = 1.0;
				}
				break;
			}
			
			case AskalItemQuantityType.STACKABLE:
			{
				tempObj = SpawnTemporaryObject(className);
				item = ItemBase.Cast(tempObj);
				if (item)
				{
					int qtyMin = item.GetQuantityMin();
					int qtyMax = item.GetQuantityMax();
					if (qtyMax <= 0)
						qtyMax = 1;
					
					minValue = Math.Max(qtyMin, 1);
					maxValue = Math.Max(qtyMax, minValue);
					stepValue = 1.0;
				}
				break;
			}
			
			case AskalItemQuantityType.QUANTIFIABLE:
			{
				// Representamos quantidades fracionárias como percentual
				minValue = 0;
				maxValue = 100;
				stepValue = 1.0;
				break;
			}
		}
		
		if (tempObj)
			GetGame().ObjectDelete(tempObj);
	}
	
	/// Configura o slider de quantidade e a seleção de conteúdo para o item selecionado
	void ConfigureQuantitySliderForItem(string className)
	{
		if (!m_TransactionQuantitySlider)
				return;
		
		// Limpa seleções anteriores
		if (m_AvailableContentTypes)
			m_AvailableContentTypes.Clear();
		if (m_AvailableLiquidTypes)
			m_AvailableLiquidTypes.Clear();
		if (m_AvailableContentUnitPrices)
			m_AvailableContentUnitPrices.Clear();
		
        m_CurrentContentIndex = 0;
		m_CurrentSelectedContent = "";
		m_CurrentItemIsLiquidContainer = false;
		m_CurrentLiquidCapacity = 0.0;
		
		AskalItemQuantityType qtyType = DetectItemQuantityType(className);
		m_SliderQuantityType = qtyType;
		m_SliderCurrentClassName = className;
		
		// Se o item não suporta quantidade variável, oculta o slider e o seletor
		if (qtyType == AskalItemQuantityType.NONE)
		{
			m_TransactionQuantitySlider.Show(false);
			if (m_TransactionContentPanel)
				m_TransactionContentPanel.Show(false);
			
			m_CurrentQuantityPercent = 100;
			m_CurrentAmmoCount = 0;
			m_SliderMinValue = 0;
			m_SliderMaxValue = 0;
			m_SliderStepValue = 1.0;
			m_SliderUnitLabel = "";
			
			UpdateSliderText();
			UpdateTransactionSummary();
			return;
		}
		
		// Obtém os valores min/max e step
		int minVal, maxVal;
		string unit;
		float stepVal;
		GetQuantityRangeForItem(className, minVal, maxVal, unit, stepVal);
		
		m_SliderMinValue = minVal;
		m_SliderMaxValue = maxVal;
		m_SliderStepValue = stepVal;
		m_SliderUnitLabel = unit;
		
		// Configura o slider
		m_TransactionQuantitySlider.Show(true);
		m_TransactionQuantitySlider.SetStep(stepVal);
		m_TransactionQuantitySlider.SetMinMax(minVal, maxVal);
		m_TransactionQuantitySlider.SetCurrent(maxVal); // Sempre inicia no máximo
		
		// Configurações específicas por tipo
		switch (qtyType)
		{
		case AskalItemQuantityType.MAGAZINE:
		{
			m_CurrentAmmoCount = maxVal;
			if (maxVal > 0)
				m_CurrentQuantityPercent = 100;
			else
				m_CurrentQuantityPercent = 0;
				
				GetCompatibleAmmoTypes(className, m_AvailableContentTypes);
				if (m_AvailableContentTypes.Count() == 0)
				{
					// Fallback para o ammo padrão do magazine
					string defaultAmmo = GetGame().ConfigGetTextOut("CfgMagazines " + className + " ammo");
					if (defaultAmmo && defaultAmmo != "" && defaultAmmo.IndexOf("Bullet_") == 0)
					{
						string ammoItemName = "Ammo_" + defaultAmmo.Substring(7, defaultAmmo.Length() - 7);
						m_AvailableContentTypes.Insert(ammoItemName);
					}
				}
				
				if (m_AvailableContentTypes.Count() > 0)
				{
					for (int i = 0; i < m_AvailableContentTypes.Count(); i++)
					{
						string ammoClass = m_AvailableContentTypes.Get(i);
						float unitPrice = ResolveAmmoUnitPrice(ammoClass);
						m_AvailableContentUnitPrices.Insert(unitPrice);
					}
					
					m_CurrentContentIndex = 0;
					m_CurrentSelectedContent = m_AvailableContentTypes.Get(0);
					
					if (m_TransactionContentPanel)
						m_TransactionContentPanel.Show(true);
					
					UpdateContentSelectorDisplay();
				}
				else
				{
					if (m_TransactionContentPanel)
						m_TransactionContentPanel.Show(false);
				}
				break;
			}
			
			case AskalItemQuantityType.STACKABLE:
			{
				m_CurrentAmmoCount = maxVal;
				m_CurrentQuantityPercent = 100;
				
				if (m_TransactionContentPanel)
					m_TransactionContentPanel.Show(false);
				break;
			}
			
			case AskalItemQuantityType.QUANTIFIABLE:
			{
				m_CurrentQuantityPercent = maxVal;
				m_CurrentAmmoCount = 0;
				
				// Detecta se é um container de líquido
				Object tempObj = SpawnTemporaryObject(className);
				ItemBase container = ItemBase.Cast(tempObj);
				
				if (container && container.IsLiquidContainer())
				{
					m_CurrentItemIsLiquidContainer = true;
					m_CurrentLiquidCapacity = Math.Max(container.GetQuantityMax(), 0.0);
					
					int defaultLiquidType = container.GetLiquidTypeInit();
					GetCompatibleLiquidTypes(className, m_AvailableLiquidTypes);
					
					if (m_AvailableLiquidTypes.Count() > 0)
					{
						for (int j = 0; j < m_AvailableLiquidTypes.Count(); j++)
						{
							int liquidType = m_AvailableLiquidTypes.Get(j);
							m_AvailableContentTypes.Insert(liquidType.ToString());
							m_AvailableContentUnitPrices.Insert(ResolveLiquidUnitPrice(liquidType));
						}
						
						int defaultIndex = m_AvailableLiquidTypes.Find(defaultLiquidType);
						if (defaultIndex < 0)
							defaultIndex = 0;
						
						m_CurrentContentIndex = defaultIndex;
						m_CurrentSelectedContent = m_AvailableContentTypes.Get(m_CurrentContentIndex);
						
						if (m_TransactionContentPanel)
							m_TransactionContentPanel.Show(true);
						
						UpdateContentSelectorDisplay();
					}
					else if (m_TransactionContentPanel)
					{
						m_TransactionContentPanel.Show(false);
					}
		}
		else
		{
					m_CurrentItemIsLiquidContainer = false;
					if (m_TransactionContentPanel)
						m_TransactionContentPanel.Show(false);
				}
				
				if (tempObj)
					GetGame().ObjectDelete(tempObj);
				break;
			}
		}
		
		UpdateSliderText();
		UpdateTransactionSummary();
		
		Print("[AskalStore] 🎚️ Slider configurado - Tipo: " + qtyType + " | Range: " + minVal + "-" + maxVal);
	}
	
	/// Callback quando o slider é movido
	void OnQuantitySliderChanged(float value)
	{
		if (!m_TransactionQuantitySlider || m_SelectedItemIndex < 0)
			return;
		
		AskalItemData itemData = m_Items.Get(m_SelectedItemIndex);
		if (!itemData)
			return;
		
		int clampedValue = Math.Round(Math.Clamp(value, m_SliderMinValue, m_SliderMaxValue));
		
		switch (m_SliderQuantityType)
		{
			case AskalItemQuantityType.MAGAZINE:
			{
				m_CurrentAmmoCount = clampedValue;
				if (m_SliderMaxValue > 0)
					m_CurrentQuantityPercent = Math.Round((m_CurrentAmmoCount / (float)m_SliderMaxValue) * 100.0);
				break;
			}
			
			case AskalItemQuantityType.STACKABLE:
			{
				m_CurrentAmmoCount = clampedValue;
				m_CurrentQuantityPercent = 100;
				break;
			}
			
			case AskalItemQuantityType.QUANTIFIABLE:
			{
				m_CurrentQuantityPercent = clampedValue;
				break;
			}
			
			default:
			{
				m_CurrentQuantityPercent = 100;
				m_CurrentAmmoCount = 0;
				break;
			}
		}
		
		UpdateSliderText();
		UpdateTransactionSummary();
	}
	
	/// Calcula o preço ajustado baseado na quantidade selecionada
	int CalculateAdjustedPrice(int basePrice, string className)
	{
		if (m_SliderQuantityType == AskalItemQuantityType.NONE)
			return basePrice;
		
		switch (m_SliderQuantityType)
		{
			case AskalItemQuantityType.MAGAZINE:
			{
				int ammoCount = Math.Clamp(m_CurrentAmmoCount, m_SliderMinValue, m_SliderMaxValue);
				float unitPrice = 0;
				if (m_AvailableContentUnitPrices && m_CurrentContentIndex >= 0 && m_CurrentContentIndex < m_AvailableContentUnitPrices.Count())
					unitPrice = m_AvailableContentUnitPrices.Get(m_CurrentContentIndex);
				
				if (unitPrice <= 0)
					unitPrice = Math.Max(1, basePrice * 0.02); // Fallback: 2% do preço do carregador
				
				return Math.Round(basePrice + (ammoCount * unitPrice));
			}
			
		case AskalItemQuantityType.STACKABLE:
		{
			int quantity = Math.Clamp(m_CurrentAmmoCount, m_SliderMinValue, m_SliderMaxValue);
			int maxQty = Math.Max(m_SliderMaxValue, 1);
			float pricePerUnit = basePrice / Math.Max(maxQty, 1);
			return Math.Round(pricePerUnit * quantity);
		}
			
			case AskalItemQuantityType.QUANTIFIABLE:
			{
				float percent = Math.Clamp(m_CurrentQuantityPercent, m_SliderMinValue, m_SliderMaxValue);
				if (m_SliderMaxValue > 0)
					percent = percent / m_SliderMaxValue;
				else
					percent = 1.0;
				
				if (m_CurrentItemIsLiquidContainer && m_AvailableContentUnitPrices && m_CurrentContentIndex >= 0 && m_CurrentContentIndex < m_AvailableContentUnitPrices.Count())
				{
					float pricePerML = m_AvailableContentUnitPrices.Get(m_CurrentContentIndex);
					if (pricePerML < 0)
						pricePerML = 0;
					float capacity = Math.Max(m_CurrentLiquidCapacity, 0.0);
					float liquidCost = capacity * percent * pricePerML;
					return Math.Round(basePrice + liquidCost);
				}
				
				return Math.Round(basePrice * percent);
			}
		}
		
		return basePrice;
	}
	
	protected void UpdateSliderText()
	{
		if (!m_TransactionQuantitySlider || !m_TransactionQuantitySliderText)
			return;
		
		float currentValue = m_TransactionQuantitySlider.GetCurrent();
		string displayText;
		
		if (m_SliderStepValue < 1.0)
		{
			float rounded = Math.Round(currentValue * 10.0) / 10.0;
			displayText = rounded.ToString();
		}
		else
		{
			displayText = Math.Round(currentValue).ToString();
		}
		
		m_TransactionQuantitySliderText.SetText(displayText);
	}
	
	protected void UpdateContentSelectorDisplay()
	{
		if (!m_TransactionContentLabel || !m_AvailableContentTypes || m_AvailableContentTypes.Count() == 0)
			return;
		
		if (m_CurrentContentIndex < 0)
			m_CurrentContentIndex = 0;
		if (m_CurrentContentIndex >= m_AvailableContentTypes.Count())
			m_CurrentContentIndex = m_AvailableContentTypes.Count() - 1;
		
		string contentKey = m_AvailableContentTypes.Get(m_CurrentContentIndex);
		string displayName = contentKey;
		
		if (m_SliderQuantityType == AskalItemQuantityType.MAGAZINE)
		{
			displayName = GetAmmoDisplayName(contentKey);
		}
		else if (m_SliderQuantityType == AskalItemQuantityType.QUANTIFIABLE && m_CurrentItemIsLiquidContainer && m_AvailableLiquidTypes && m_CurrentContentIndex < m_AvailableLiquidTypes.Count())
		{
			int liquidType = m_AvailableLiquidTypes.Get(m_CurrentContentIndex);
			displayName = GetLiquidDisplayName(liquidType);
		}
		
		if (!displayName || displayName == "")
			displayName = "—";
		
		m_TransactionContentLabel.SetText(displayName);
		m_CurrentSelectedContent = contentKey;
	}
	
	protected void OnContentSelectorPrevClick()
	{
		if (!m_AvailableContentTypes || m_AvailableContentTypes.Count() == 0)
			return;
		
		m_CurrentContentIndex--;
		if (m_CurrentContentIndex < 0)
			m_CurrentContentIndex = m_AvailableContentTypes.Count() - 1;
		
		UpdateContentSelectorDisplay();
		UpdateTransactionSummary();
	}
	
	protected void OnContentSelectorNextClick()
	{
		if (!m_AvailableContentTypes || m_AvailableContentTypes.Count() == 0)
			return;
		
		m_CurrentContentIndex++;
		if (m_CurrentContentIndex >= m_AvailableContentTypes.Count())
			m_CurrentContentIndex = 0;
		
		UpdateContentSelectorDisplay();
		UpdateTransactionSummary();
	}
	
	void OnPurchaseClick(ButtonWidget sourceButton = null)
	{
		ButtonWidget actionButton = sourceButton;
		if (!actionButton)
			actionButton = GetActiveBuyButton();
		if (!actionButton)
			return;
		
		if (IsButtonCoolingDown(actionButton))
		{
			Print("[AskalStore] ⏱️ Compra bloqueada durante cooldown");
			ShowNotification("Aguarde o cooldown para realizar outra transação", "#FFAA00");
			DisplayTransactionError("Aguarde o cooldown para realizar outra transação");
			return;
		}
		
		if (m_BatchBuyEnabled)
		{
			if (ProcessBatchPurchase(actionButton))
				StartButtonCooldown(actionButton);
			return;
		}
		
		if (m_SelectedItemIndex < 0 || m_SelectedItemIndex >= m_Items.Count())
		{
			Print("[AskalStore] ⚠️ Nenhum item selecionado");
			ShowNotification("Selecione um item para comprar", "#FF0000");
			DisplayTransactionError("Selecione um item para comprar");
			return;
		}
		
		AskalItemData itemData = m_Items.Get(m_SelectedItemIndex);
		if (!itemData)
		{
			Print("[AskalStore] ⚠️ Item inválido");
			DisplayTransactionError("Item selecionado inválido");
			return;
		}
		
		string itemClass = itemData.GetClassName();
		int price = itemData.GetPrice();
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		string currencyId = m_ActiveCurrencyId;
		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
		if (m_SelectedVariantClass != "")
		{
			itemClass = m_SelectedVariantClass;
			AskalItemSyncData variantData = NULL;
			if (cache)
			{
				variantData = cache.FindItem(itemClass);
			}
			array<string> variantAttachments = BuildAttachmentList(variantData, cache, itemClass);
			int variantBasePrice = 0;
			if (variantData)
			{
				variantBasePrice = variantData.BasePrice;
			}
			if (variantBasePrice <= 0)
				variantBasePrice = itemData.GetBasePrice();
			variantBasePrice = NormalizeBuyPrice(variantBasePrice, itemData.GetBasePrice());
			int variantTotalPrice = ComputeTotalItemPrice(variantBasePrice, variantAttachments, cache);
			variantTotalPrice = NormalizeBuyPrice(variantTotalPrice, variantBasePrice);
			variantTotalPrice = ApplyBuyCoefficient(variantTotalPrice);
			if (variantTotalPrice > 0)
				price = variantTotalPrice;
		}
		
		string displayName = itemData.GetDisplayName();
		if (displayName == "")
			displayName = itemClass;
		
		m_SelectedItemUnitPrice = price;
		int totalPrice = price * m_TransactionQuantity;
		Print("[AskalStore] 💰 Tentando comprar: " + itemClass + " | Quantidade: " + m_TransactionQuantity + " | Total: " + totalPrice + " " + currencyId);
		
		if (GetGame().IsClient())
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			if (!player)
			{
				Print("[AskalStore] ❌ Player não encontrado");
				DisplayTransactionError("Player não encontrado para executar a compra");
				return;
			}
			
			PlayerIdentity identity = player.GetIdentity();
			if (!identity)
			{
				Print("[AskalStore] ❌ Player identity não encontrada");
				DisplayTransactionError("Identidade do player não encontrada");
				return;
			}
			
			string steamId = identity.GetPlainId();
			if (!steamId || steamId == "")
				steamId = identity.GetId();
			
			float itemQuantity = -1;
			int itemContent = 0;
			
			if (m_SliderQuantityType != AskalItemQuantityType.NONE)
			{
				switch (m_SliderQuantityType)
				{
					case AskalItemQuantityType.MAGAZINE:
					{
						itemQuantity = m_CurrentAmmoCount;
						break;
					}
					case AskalItemQuantityType.STACKABLE:
					{
						itemQuantity = m_CurrentAmmoCount;
						break;
					}
					case AskalItemQuantityType.QUANTIFIABLE:
					{
						itemQuantity = m_CurrentQuantityPercent;
						itemContent = m_CurrentSelectedContent.ToInt();
						break;
					}
				}
			}
			
			string traderName = m_CurrentTraderName;
			if (!traderName || traderName == "")
				traderName = "";
			
			Param8<string, string, int, string, float, int, int, string> params = new Param8<string, string, int, string, float, int, int, string>(steamId, itemClass, totalPrice, currencyId, itemQuantity, m_SliderQuantityType, itemContent, traderName);
			GetRPCManager().SendRPC("AskalPurchaseModule", "PurchaseItemRequest", params, true, identity, NULL);
			Print("[AskalStore] 📤 RPC de compra enviado | Qty: " + m_TransactionQuantity + " | QtyType: " + m_SliderQuantityType + " | Content: " + itemContent + " | Trader: " + traderName);
		}
		
		StartButtonCooldown(actionButton);
	}
	
	protected bool ProcessBatchPurchase(ButtonWidget sourceButton = null)
	{
		ButtonWidget actionButton = sourceButton;
		if (!actionButton)
			actionButton = GetActiveBuyButton();
		if (!actionButton)
			return false;
		
		if (!m_BatchBuySelectedIndexes || m_BatchBuySelectedIndexes.Count() == 0)
		{
			ShowNotification("Selecione itens para comprar em lote", "#FF0000");
			DisplayTransactionError("Nenhum item selecionado para compra em lote");
			return false;
		}
		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
		{
			Print("[AskalStore] ❌ Player não encontrado para compra em lote");
			DisplayTransactionError("Player não encontrado para compra em lote");
			return false;
		}
		
		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
		{
			Print("[AskalStore] ❌ Identity não encontrada para compra em lote");
			DisplayTransactionError("Identidade não encontrada para compra em lote");
			return false;
		}
		
		string steamId = identity.GetPlainId();
		if (!steamId || steamId == "")
			steamId = identity.GetId();
		
		string currencyId = m_ActiveCurrencyId;
		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
		ref array<ref AskalPurchaseRequestData> requests = new array<ref AskalPurchaseRequestData>();
		for (int idx = 0; idx < m_BatchBuySelectedIndexes.Count(); idx++)
		{
			int itemIndex = m_BatchBuySelectedIndexes.GetKey(idx);
			if (itemIndex < 0 || itemIndex >= m_Items.Count())
				continue;
			
			AskalItemData itemData = m_Items.Get(itemIndex);
			if (!itemData)
				continue;
			
			string itemClass = itemData.GetClassName();
			if (!itemClass || itemClass == "")
				continue;
			
			AskalPurchaseRequestData request = new AskalPurchaseRequestData();
			request.ItemClass = itemClass;
			request.Price = itemData.GetPrice();
			request.Quantity = -1.0;
			request.QuantityType = AskalItemQuantityType.NONE;
			request.ContentType = 0;
			requests.Insert(request);
		}
		
		if (!requests || requests.Count() == 0)
		{
			DisplayTransactionError("Nenhum item válido para comprar em lote");
			return false;
		}
		
		Param3<string, string, ref array<ref AskalPurchaseRequestData>> batchParams = new Param3<string, string, ref array<ref AskalPurchaseRequestData>>(steamId, currencyId, requests);
		GetRPCManager().SendRPC("AskalPurchaseModule", "PurchaseBatchRequest", batchParams, true, identity, NULL);
		Print("[AskalStore] 📤 RPC de compra em lote enviado com " + requests.Count() + " itens");
		
		ClearBatchSelections(true);
		UpdateTransactionSummary();
		DisplayTransactionMessage("Solicitação de compra em lote enviada");
		return true;
	}
	
	void OnSellClick(ButtonWidget sourceButton = null)
	{
		ButtonWidget actionButton = sourceButton;
		if (!actionButton)
			actionButton = GetActiveSellButton();
		if (!actionButton)
			return;
		
		if (IsButtonCoolingDown(actionButton))
		{
			Print("[AskalStore] ⏱️ Venda bloqueada durante cooldown");
			ShowNotification("Aguarde o cooldown antes de tentar novamente", "#FFAA00");
			DisplayTransactionError("Aguarde o cooldown para realizar outra transação");
			return;
		}
		
		if (m_BatchSellEnabled)
		{
			if (ProcessBatchSell(actionButton))
				StartButtonCooldown(actionButton);
			return;
		}
		
		if (ProcessSingleSell(actionButton))
			StartButtonCooldown(actionButton);
	}
	
	protected bool ProcessSingleSell(ButtonWidget sourceButton = null)
	{
		EntityAI itemToSell = m_SelectedInventoryItem;
		
		string itemTypeStr = "NULL";
		if (itemToSell)
			itemTypeStr = itemToSell.GetType();
		Print("[AskalStore]   m_SelectedInventoryItem: " + itemTypeStr);
		Print("[AskalStore]   m_ShowingInventoryForSale: " + m_ShowingInventoryForSale);
		Print("[AskalStore]   m_SelectedItemIndex: " + m_SelectedItemIndex);
		
		if (!itemToSell && m_ShowingInventoryForSale)
		{
			AskalInventoryDisplayInfo info = GetInventoryDisplayInfo(m_SelectedItemIndex);
			if (info && info.Item)
			{
				itemToSell = info.Item;
			}
		}
		
		// Se ainda não tem item, tentar obter do className atual
		if (!itemToSell && m_CurrentSelectedClassName != "")
		{
			itemToSell = GetFirstItemInInventory(m_CurrentSelectedClassName);
		}
		
		if (!itemToSell)
		{
			Print("[AskalStore] [ERRO] Nenhum item encontrado para vender");
			ShowNotification("Selecione um item do inventário para vender", "#FF0000");
			DisplayTransactionError("Selecione um item do inventário para vender");
			return false;
		}
		
		
		if (!SendSellRequest(itemToSell))
			return false;
		
		m_SelectedInventoryItem = null;
		ScanPlayerInventory();
		if (m_BatchSellEnabled || m_ShowingInventoryForSale)
			RenderInventoryItemsForSale();
		UpdateTransactionSummary();
		return true;
	}
	
	// Verificar se item tem cargo (itens dentro de containers) - versão cliente
	// Versão interna com proteção contra loops infinitos
	protected bool HasCargoItemsRecursiveInternal(EntityAI item, array<EntityAI> checkedItems)
	{
		if (!item || !item.GetInventory())
			return false;
		
		// Proteção contra loops infinitos: verificar se já foi checado
		if (checkedItems.Find(item) != -1)
			return false; // Já foi verificado, evitar loop
		
		checkedItems.Insert(item);
		
		// Primeiro, verificar attachments para excluí-los
		array<EntityAI> attachments = new array<EntityAI>();
		int attCount = item.GetInventory().AttachmentCount();
		for (int attIdx = 0; attIdx < attCount; attIdx++)
		{
			EntityAI attachment = item.GetInventory().GetAttachmentFromIndex(attIdx);
			if (attachment)
				attachments.Insert(attachment);
		}
		
		array<EntityAI> allItems = new array<EntityAI>();
		item.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, allItems);
		
		// Filtrar apenas itens que estão no cargo (não attachments, não o próprio item)
		foreach (EntityAI cargoItem : allItems)
		{
			if (!cargoItem)
				continue;
			
			// Ignorar o próprio item
			if (cargoItem == item)
				continue;
			
			// Verificar se é attachment
			if (attachments.Find(cargoItem) != -1)
				continue; // É attachment, não conta como cargo
			
			// Verificar usando InventoryLocation se o item está realmente dentro deste container
			InventoryLocation itemLoc = new InventoryLocation();
			if (!cargoItem.GetInventory().GetCurrentInventoryLocation(itemLoc))
				continue; // Não conseguiu obter location, pular
			
			// Se o parent não é o item sendo verificado, não é cargo deste item
			EntityAI parent = itemLoc.GetParent();
			if (parent != item)
				continue; // Item não está dentro deste container
			
			// Item está no cargo - verificar recursivamente se ele também tem cargo
			if (HasCargoItemsRecursiveInternal(cargoItem, checkedItems))
				return true; // Container dentro de container com itens
			
			// Item no cargo encontrado
			return true;
		}
		
		return false; // Sem itens no cargo
	}
	
	// Versão pública (wrapper)
	protected bool HasCargoItemsRecursive(EntityAI item)
	{
		array<EntityAI> checkedItems = new array<EntityAI>();
		return HasCargoItemsRecursiveInternal(item, checkedItems);
	}
	
	protected bool SendSellRequest(EntityAI inventoryItem)
	{
		if (!inventoryItem)
		{
			Print("[AskalStore] [ERRO] SendSellRequest: inventoryItem é NULL");
			return false;
		}
		
		// Verificar se item tem cargo ANTES de enviar para o servidor
		if (HasCargoItemsRecursive(inventoryItem))
		{
			Print("[AskalStore] [ERRO] Item tem cargo - venda bloqueada no cliente");
			DisplayTransactionError("[AVISO] Item ocupado: esvazie o inventario do item antes de vende-lo");
			return false;
		}
		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
		{
			Print("[AskalStore] [ERRO] Player não encontrado para venda");
			DisplayTransactionError("Player não encontrado para realizar a venda");
			return false;
		}
		
		PlayerIdentity identity = player.GetIdentity();
		if (!identity)
		{
			Print("[AskalStore] [ERRO] Identity não encontrada para venda");
			DisplayTransactionError("Identidade do player não encontrada para venda");
			return false;
		}
		
		string steamId = identity.GetPlainId();
		if (!steamId || steamId == "")
			steamId = identity.GetId();
		
		string currencyId = m_ActiveCurrencyId;
		if (!currencyId || currencyId == "")
			currencyId = "Askal_Coin";
		
		int transactionMode = 1;
		string traderName = m_CurrentTraderName;
		if (!traderName || traderName == "")
			traderName = "Trader_Default";
		
		// IMPORTANTE: Usar GetType() diretamente do item do inventário para garantir nome exato
		string itemClassName = inventoryItem.GetType();
		
		
		// Verificar se o item realmente existe no inventário do player
		if (!IsItemInInventory(itemClassName))
		{
			Print("[AskalStore] [AVISO] Item não encontrado no cache do inventário: " + itemClassName);
			Print("[AskalStore] [AVISO] Tentando normalizar nome...");
			string normalized = NormalizeClassName(itemClassName);
			if (IsItemInInventory(normalized))
			{
				Print("[AskalStore] [AVISO] Item encontrado com nome normalizado: " + normalized);
				// Tentar encontrar o item real no inventário
				EntityAI realItem = GetFirstItemInInventory(normalized);
				if (realItem)
				{
					itemClassName = realItem.GetType();
					Print("[AskalStore] [AVISO] Usando nome do item real: " + itemClassName);
				}
			}
		}
		
		Param5<string, string, string, int, string> sellParams = new Param5<string, string, string, int, string>(steamId, itemClassName, currencyId, transactionMode, traderName);
		
		GetRPCManager().SendRPC("AskalSellModule", "SellItemRequest", sellParams, true, identity, NULL);
		Print("[AskalStore] [VENDA] RPC de venda enviado para: " + itemClassName);
		
		DisplayTransactionMessage("Solicitação de venda enviada para " + itemClassName);
		return true;
	}
	
	// ========================================
	// SISTEMA DE NOTIFICAÇÕES
	// ========================================
	
	// Adicionar notificação com descrição detalhada
	void AddNotificationWithDescription(string actionType, string description, string itemClassName, int price, bool isPurchase = true)
	{
		if (!m_NotificationCardHolder)
		{
			Print("[AskalStore] ⚠️ NotificationCardHolder não disponível!");
			return;
		}
		
		// Limpar notificações antigas
		CleanupOldNotifications();
		
		// Limitar número máximo de notificações
		if (m_NotificationCards.Count() >= MAX_NOTIFICATIONS)
		{
			RemoveOldestNotification();
		}
		
		// Criar card de notificação
		Widget notificationCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_notification_card.layout", m_NotificationCardHolder);
		if (!notificationCard)
		{
			Print("[AskalStore] ❌ Falha ao criar notification card!");
			return;
		}
		
		Widget slidePanel = notificationCard.FindAnyWidget("notification_slide_panel");
		Widget colorTarget = slidePanel;
		if (!colorTarget)
			colorTarget = notificationCard;
		
		// Configurar cor baseada no tipo (compra = verde, venda = laranja)
		if (isPurchase)
			colorTarget.SetColor(ARGB(200, 9, 116, 0)); // Verde para compra
		else
			colorTarget.SetColor(ARGB(200, 131, 67, 0)); // Laranja para venda
		
		float basePosX = 0;
		float basePosY = 0;
		float parentWidthPx = 0;
		if (slidePanel)
		{
			float currentPosX;
			slidePanel.GetPos(currentPosX, basePosY);
			Widget slideParent = slidePanel.GetParent();
			float parentHeightPx;
			if (slideParent)
				slideParent.GetScreenSize(parentWidthPx, parentHeightPx);
			if (parentWidthPx <= 0)
			{
				slidePanel.GetScreenSize(parentWidthPx, parentHeightPx);
			}
			if (parentWidthPx <= 0)
				parentWidthPx = 70.0;
			float initialOffset = 70.0;
			float initialOffsetNormalized = initialOffset / parentWidthPx;
			slidePanel.SetPos(initialOffsetNormalized, basePosY);
			slidePanel.Update();
		}
		
		// Configurar texto de ação
		MultilineTextWidget actionText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_text"));
		if (actionText)
		{
			actionText.SetText(actionType);
		}
		
		// Configurar nome/descrição do item (usar descrição detalhada)
		MultilineTextWidget itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_item_name_text"));
		if (!itemNameText)
			itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("item_name_text"));
		if (!itemNameText)
			itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("description_text"));
		
		if (itemNameText)
		{
			itemNameText.SetText(description);
		}
		
		// Configurar preview do item (usar className original)
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(notificationCard.FindAnyWidget("action_item_preview"));
		if (itemPreview && itemClassName != "")
		{
			EntityAI previewEntity = EntityAI.Cast(SpawnTemporaryObject(itemClassName));
			if (previewEntity)
			{
				itemPreview.SetItem(previewEntity);
				itemPreview.SetModelPosition(Vector(0, 0, 0.5));
				itemPreview.SetView(previewEntity.GetViewIndex());
				itemPreview.Show(true);
				m_PreviewItems.Insert(previewEntity);
			}
		}
		
		// Configurar preço
		MultilineTextWidget priceText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_price"));
		if (priceText)
		{
			string formattedPrice = FormatCurrencyValue(price);
			string priceStr = formattedPrice;
			if (isPurchase && price > 0)
				priceStr = "-" + formattedPrice;
			else if (!isPurchase && price > 0)
				priceStr = "+" + formattedPrice;
			
			if (m_CurrentCurrencyShortName && m_CurrentCurrencyShortName != "")
				priceStr = priceStr + " " + m_CurrentCurrencyShortName;
			
			priceText.SetText(priceStr);
		}
		
		// Adicionar à lista
		m_NotificationCards.Insert(notificationCard);
		float animationStartTime = GetGame().GetTickTime();
		m_NotificationTimestamps.Insert(animationStartTime);
		if (slidePanel)
		{
			m_NotificationSlidePanels.Insert(slidePanel);
			m_NotificationAnimationStartTimes.Insert(animationStartTime);
			m_NotificationBasePosY.Insert(basePosY);
			m_NotificationSlideParentWidths.Insert(parentWidthPx);
		}
	}
	
	void AddNotification(string actionType, string itemClassName, int price, bool isPurchase = true)
	{
		if (!m_NotificationCardHolder)
		{
			Print("[AskalStore] ⚠️ NotificationCardHolder não disponível!");
			return;
		}
		
		// Limpar notificações antigas
		CleanupOldNotifications();
		
		// Limitar número máximo de notificações
		if (m_NotificationCards.Count() >= MAX_NOTIFICATIONS)
		{
			RemoveOldestNotification();
		}
		
		// Criar card de notificação
		Widget notificationCard = GetGame().GetWorkspace().CreateWidgets("askal/market/gui/new_layouts/askal_store_notification_card.layout", m_NotificationCardHolder);
		if (!notificationCard)
		{
			Print("[AskalStore] ❌ Falha ao criar notification card!");
			return;
		}
		
		Widget slidePanel = notificationCard.FindAnyWidget("notification_slide_panel");
		Widget colorTarget = slidePanel;
		if (!colorTarget)
			colorTarget = notificationCard;
		
		// Configurar cor baseada no tipo (compra = verde, venda = laranja)
		if (isPurchase)
			colorTarget.SetColor(ARGB(200, 9, 116, 0)); // Verde para compra
		else
			colorTarget.SetColor(ARGB(200, 131, 67, 0)); // Laranja para venda
		
		float basePosX = 0;
		float basePosY = 0;
		float parentWidthPx = 0;
		if (slidePanel)
		{
			float currentPosX;
			slidePanel.GetPos(currentPosX, basePosY);
			Widget slideParent = slidePanel.GetParent();
			float parentHeightPx;
			if (slideParent)
				slideParent.GetScreenSize(parentWidthPx, parentHeightPx);
			if (parentWidthPx <= 0)
			{
				slidePanel.GetScreenSize(parentWidthPx, parentHeightPx);
			}
			if (parentWidthPx <= 0)
				parentWidthPx = 70.0;
			float initialOffset = 70.0;
			float initialOffsetNormalized = initialOffset / parentWidthPx;
			slidePanel.SetPos(initialOffsetNormalized, basePosY);
			slidePanel.Update();
		}
		
		// Configurar texto de ação
		MultilineTextWidget actionText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_text"));
		if (actionText)
		{
			actionText.SetText(actionType);
		}
		
		// Configurar nome/descrição do item (se o widget existir)
		// itemClassName agora pode conter descrição detalhada (com attachments)
		MultilineTextWidget itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_item_name_text"));
		if (!itemNameText)
			itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("item_name_text"));
		if (!itemNameText)
			itemNameText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("description_text"));
		
		if (itemNameText)
		{
			itemNameText.SetText(itemClassName);
		}
		
		// Configurar preview do item
		// Para preview, precisamos do className original (não a descrição)
		// Se itemClassName for uma descrição, vamos tentar usar o className do notifData
		string previewClassName = itemClassName;
		
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(notificationCard.FindAnyWidget("action_item_preview"));
		if (itemPreview && previewClassName != "")
		{
			// Tentar criar preview com o className (se for className válido)
			// Se não funcionar, a descrição será exibida no texto mas sem preview
			EntityAI previewEntity = EntityAI.Cast(SpawnTemporaryObject(previewClassName));
			if (previewEntity)
			{
				itemPreview.SetItem(previewEntity);
				itemPreview.SetModelPosition(Vector(0, 0, 0.5));
				itemPreview.SetView(previewEntity.GetViewIndex());
				itemPreview.Show(true);
				m_PreviewItems.Insert(previewEntity);
			}
		}
		
		// Configurar preço
		MultilineTextWidget priceText = MultilineTextWidget.Cast(notificationCard.FindAnyWidget("action_price"));
		if (priceText)
		{
			string formattedPrice = FormatCurrencyValue(price);
			string priceStr = formattedPrice;
			if (isPurchase && price > 0)
				priceStr = "-" + formattedPrice;
			else if (!isPurchase && price > 0)
				priceStr = "+" + formattedPrice;
			
			if (m_CurrentCurrencyShortName && m_CurrentCurrencyShortName != "")
				priceStr = priceStr + " " + m_CurrentCurrencyShortName;
			
			priceText.SetText(priceStr);
		}
		
		// Adicionar à lista
		m_NotificationCards.Insert(notificationCard);
		float animationStartTime = GetGame().GetTickTime();
		m_NotificationTimestamps.Insert(animationStartTime);
		if (slidePanel)
		{
			m_NotificationSlidePanels.Insert(slidePanel);
			m_NotificationAnimationStartTimes.Insert(animationStartTime);
			m_NotificationBasePosY.Insert(basePosY);
			m_NotificationSlideParentWidths.Insert(parentWidthPx);
		}
		else
		{
			m_NotificationSlidePanels.Insert(NULL);
			m_NotificationAnimationStartTimes.Insert(-1);
			m_NotificationBasePosY.Insert(0);
			m_NotificationSlideParentWidths.Insert(0);
		}
		
		// Atualizar holder
		if (m_NotificationCardHolder)
			m_NotificationCardHolder.Update();
		
		Print("[AskalStore] ✅ Notificação adicionada: " + actionType + " - " + itemClassName + " (" + price + ")");
	}
	
	void CleanupOldNotifications()
	{
		if (!m_NotificationCards || !m_NotificationTimestamps)
			return;
		
		float currentTime = GetGame().GetTickTime();
		array<int> indicesToRemove = new array<int>();
		
		for (int i = m_NotificationTimestamps.Count() - 1; i >= 0; i--)
		{
			float age = currentTime - m_NotificationTimestamps.Get(i);
			if (age > NOTIFICATION_LIFETIME)
			{
				indicesToRemove.Insert(i);
			}
		}
		
		foreach (int idx : indicesToRemove)
		{
			RemoveNotificationByIndex(idx);
		}
	}
	
	void RemoveOldestNotification()
	{
		if (m_NotificationCards && m_NotificationCards.Count() > 0)
		{
			RemoveNotificationByIndex(0);
		}
	}
	
	void RemoveNotificationByIndex(int index)
	{
		if (!m_NotificationCards || !m_NotificationTimestamps)
			return;
		
		if (index < 0 || index >= m_NotificationCards.Count())
			return;
		
		Widget card = m_NotificationCards.Get(index);
		if (card)
		{
			delete card;
		}
		
		m_NotificationCards.Remove(index);
		m_NotificationTimestamps.Remove(index);
		
		if (m_NotificationSlidePanels && index < m_NotificationSlidePanels.Count())
			m_NotificationSlidePanels.Remove(index);
		if (m_NotificationAnimationStartTimes && index < m_NotificationAnimationStartTimes.Count())
			m_NotificationAnimationStartTimes.Remove(index);
		if (m_NotificationBasePosY && index < m_NotificationBasePosY.Count())
			m_NotificationBasePosY.Remove(index);
		if (m_NotificationSlideParentWidths && index < m_NotificationSlideParentWidths.Count())
			m_NotificationSlideParentWidths.Remove(index);
	}
	
	void ClearAllNotifications()
	{
		if (!m_NotificationCards)
			return;
		
		foreach (Widget card : m_NotificationCards)
		{
			if (card)
				delete card;
		}
		
		m_NotificationCards.Clear();
		if (m_NotificationTimestamps)
			m_NotificationTimestamps.Clear();
		if (m_NotificationSlidePanels)
			m_NotificationSlidePanels.Clear();
		if (m_NotificationAnimationStartTimes)
			m_NotificationAnimationStartTimes.Clear();
		if (m_NotificationBasePosY)
			m_NotificationBasePosY.Clear();
		if (m_NotificationSlideParentWidths)
			m_NotificationSlideParentWidths.Clear();

		if (m_NotificationCardHolder)
			m_NotificationCardHolder.Update();
	}
	
	void ShowNotification(string message, string color = "#FFFFFF")
	{
		Print("[AskalStore] " + message);
	}
	
	// Processar notificações pendentes do helper (3_Game)
	void ProcessPendingNotifications()
	{
		if (!s_Instance)
			return;
		
		array<ref AskalNotificationData> pending = AskalNotificationHelper.GetPendingNotifications();
		if (!pending || pending.Count() == 0)
			return;
		
		bool inventoryNeedsUpdate = false;
		
		// Processar todas as notificações pendentes
		for (int i = pending.Count() - 1; i >= 0; i--)
		{
			AskalNotificationData notifData = pending.Get(i);
			if (notifData)
			{
				// Usar descrição detalhada se disponível, senão usar className
				string displayText = notifData.Description;
				if (!displayText || displayText == "")
					displayText = notifData.ItemClassName;
				
				// Passar descrição para exibição e className original para preview
				AddNotificationWithDescription(notifData.ActionType, displayText, notifData.ItemClassName, notifData.Price, notifData.IsPurchase);
				
				// Se for uma venda, marcar para atualizar inventário
				if (!notifData.IsPurchase)
					inventoryNeedsUpdate = true;
				
				AskalNotificationHelper.RemoveNotification(i);
			}
		}
		
		// Atualizar inventário após vendas processadas
		if (inventoryNeedsUpdate)
		{
			ScanPlayerInventory();
			if (m_BatchSellEnabled || m_ShowingInventoryForSale)
				RenderInventoryItemsForSale();
			UpdateTransactionSummary();
		}
	}
	
	
	// ========================================
	// ON SHOW / ON HIDE
	// ========================================
	
	// Abrir menu do trader (chamado via RPC)
	void OpenTraderMenu(string traderName = "")
	{
		if (traderName != "")
		{
			Print("[AskalStore] 🏪 Abrindo menu do trader: " + traderName);
			
			// Armazenar nome do trader
			m_CurrentTraderName = traderName;
			
			// Obter SetupItems do helper
			m_TraderSetupItems = AskalNotificationHelper.GetPendingTraderSetupItems();
			
			// Resolve currency for trader
			AskalCurrencyConfig resolvedCurrencyCfg = NULL;
			string resolvedCurrencyId = "";
			if (AskalMarketConfig.ResolveAcceptedCurrency(traderName, "", resolvedCurrencyId, resolvedCurrencyCfg))
			{
				m_ActiveCurrencyId = resolvedCurrencyId;
				Print("[AskalStore] 💰 Currency resolvida para trader " + traderName + ": " + resolvedCurrencyId);
			}
			else
			{
				// Fallback to default
				AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
				if (marketConfig)
					m_ActiveCurrencyId = marketConfig.GetDefaultCurrencyId();
				Print("[AskalStore] ⚠️ Usando currency padrão: " + m_ActiveCurrencyId);
			}
			RefreshCurrencyShortname();
			
			// Atualizar título do menu
			if (m_HeaderTitleText)
			{
				m_HeaderTitleText.SetText(traderName);
				Print("[AskalStore] ✅ Título atualizado: " + traderName);
			}
			
			// Recarregar datasets com filtros do trader
			LoadDatasetsFromCore();
			
			// Recarregar primeira categoria se houver datasets
			if (m_Datasets.Count() > 0)
			{
				LoadDataset(0);
				if (m_Categories.Count() > 0)
					LoadCategory(0);
			}
		}
		
		// Se o menu já existe, apenas garantir que está visível
		// Caso contrário, será criado e mostrado pelo UIManager quando necessário
		if (s_Instance && s_Instance == this)
		{
			Print("[AskalStore] ✅ Menu já está aberto");
		}
		else
		{
			// Criar nova instância e mostrar via UIManager
			AskalStoreMenu newMenu = new AskalStoreMenu();
			if (newMenu)
			{
				GetGame().GetUIManager().ShowScriptedMenu(newMenu, NULL);
				Print("[AskalStore] ✅ Menu do trader aberto");
			}
		}
	}
	
	override void OnShow()
	{
		super.OnShow();
		
		// Verificar se há solicitação de abertura de menu do trader pendente
		string pendingTraderMenu = AskalNotificationHelper.GetPendingTraderMenu();
		if (pendingTraderMenu && pendingTraderMenu != "")
		{
			Print("[AskalStore] 🏪 OnShow: Processando trader pendente: " + pendingTraderMenu);
			
			// Armazenar nome do trader
			m_CurrentTraderName = pendingTraderMenu;
			
			// Obter SetupItems do helper
			m_TraderSetupItems = AskalNotificationHelper.GetPendingTraderSetupItems();
			
			// Atualizar título do menu
			if (m_HeaderTitleText)
			{
				m_HeaderTitleText.SetText(pendingTraderMenu);
				Print("[AskalStore] ✅ Título atualizado para: " + pendingTraderMenu);
			}
			else
			{
				Print("[AskalStore] ⚠️ m_HeaderTitleText é NULL em OnShow(), tentando encontrar...");
				// Tentar encontrar novamente
				if (m_RootWidget)
				{
					m_HeaderTitleText = TextWidget.Cast(m_RootWidget.FindAnyWidget("market_header_title_text"));
					if (m_HeaderTitleText)
					{
						m_HeaderTitleText.SetText(pendingTraderMenu);
						Print("[AskalStore] ✅ Título encontrado e atualizado: " + pendingTraderMenu);
					}
					else
					{
						Print("[AskalStore] ❌ market_header_title_text não encontrado no layout!");
					}
				}
			}
			
			// Chamar OpenTraderMenu para aplicar configuração
			OpenTraderMenu(pendingTraderMenu);
			AskalNotificationHelper.ClearPendingTraderMenu();
		}
		
		// Desativar controles primeiro
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		
		// Tentar encontrar e ocultar NotificationUI através do workspace para evitar spam de erros
		// O NotificationUI está dentro do HUD, então tentamos acessá-lo antes de ocultar
		Widget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			// Tentar encontrar o widget do NotificationUI (pode ter vários nomes possíveis)
			Widget notificationPanel = workspace.FindAnyWidget("NotificationPanel");
			if (!notificationPanel)
				notificationPanel = workspace.FindAnyWidget("NotificationsPanel");
			if (!notificationPanel)
				notificationPanel = workspace.FindAnyWidget("NotificationUI");
			
			if (notificationPanel)
			{
				notificationPanel.Show(false);
				Print("[AskalStore] ✅ NotificationUI desabilitado");
			}
			// Se não encontrou, os erros serão apenas spam no log, não fatal
		}
		
		// Ocultar HUD
		Mission mis = GetGame().GetMission();
		if (mis && mis.GetHud())
		{
			mis.GetHud().Show(false);
			Print("[AskalStore] ✅ HUD ocultada");
		}
	}
	
	override void OnHide()
	{
		super.OnHide();
		
		ClearItems();
		ClearAllNotifications();
		
		// Limpar instância estática
		if (s_Instance == this)
			s_Instance = NULL;
		
		// Limpar hover panel
		if (m_HoverPanel)
		{
			delete m_HoverPanel;
			m_HoverPanel = NULL;
			m_HoverText = NULL;
		}
		
		// Restaurar HUD primeiro
		Mission mis = GetGame().GetMission();
		if (mis && mis.GetHud())
		{
			mis.GetHud().Show(true);
			Print("[AskalStore] ✅ HUD restaurada");
		}
		
		// Restaurar NotificationUI se foi desabilitado
		Widget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			Widget notificationPanel = workspace.FindAnyWidget("NotificationPanel");
			if (!notificationPanel)
				notificationPanel = workspace.FindAnyWidget("NotificationsPanel");
			if (!notificationPanel)
				notificationPanel = workspace.FindAnyWidget("NotificationUI");
			
			if (notificationPanel)
			{
				notificationPanel.Show(true);
				Print("[AskalStore] ✅ NotificationUI restaurado");
			}
		}
		
		// Restaurar controles
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().PlayerControlEnable(true);

		ResetAllCooldowns();
	}
	
	// ========================================
	// UPDATE (ROTAÇÃO DO PREVIEW + VERIFICAÇÃO DE SINCRONIZAÇÃO)
	// ========================================
	
	protected bool m_HasRequestedDatasets = false;
	protected float m_LastSyncCheck = 0.0;
	protected const float SYNC_CHECK_INTERVAL = 1.0; // Verificar a cada 1 segundo
	
	// ========================================
	// SISTEMA DE COOLDOWN E BOTÕES DE AÇÃO
	// ========================================
	
	/// Atualiza visibilidade dos botões de ação baseado no item mode
	void UpdateActionButtons(int itemMode)
	{
		// itemMode: -1=Disabled, 0=See Only, 1=Buy Only, 2=Sell Only, 3=Buy+Sell
		m_CurrentItemMode = itemMode;
		
		bool canBuy = (itemMode == 1 || itemMode == 3);
		bool canSell = (itemMode == 2 || itemMode == 3);
		
		bool playerHasItem = false;
		if (m_BatchSellEnabled)
			playerHasItem = true;
		else if (m_SelectedInventoryItem)
			playerHasItem = true;
		else if (m_CurrentSelectedClassName && m_CurrentSelectedClassName != "")
			playerHasItem = IsItemInInventory(m_CurrentSelectedClassName);
		
		if (canSell && !playerHasItem)
			canSell = false;
		if (itemMode <= 0)
		{
			canBuy = false;
			if (itemMode <= 0)
				canSell = false;
		}
		
		m_CurrentCanBuy = canBuy;
		m_CurrentCanSell = canSell;
		
		bool showDual = canBuy && canSell;
		bool showBuySolo = canBuy && !canSell;
		bool showSellSolo = canSell && !canBuy;
		bool showSinglePanel = (!showDual) && (showBuySolo || showSellSolo);
		
		if (m_BuySellDualPanel)
			m_BuySellDualPanel.Show(showDual);
		if (m_BuyButton)
			m_BuyButton.Show(showDual);
		if (m_SellButton)
			m_SellButton.Show(showDual);
		
		if (m_BuySellSinglePanel)
			m_BuySellSinglePanel.Show(showSinglePanel);
		
		if (m_BuyButtonSolo)
			m_BuyButtonSolo.Show(showBuySolo);
		if (m_SellButtonSolo)
			m_SellButtonSolo.Show(showSellSolo);
		
		if (!showDual && !showSinglePanel)
		{
			if (m_BuySellDualPanel)
				m_BuySellDualPanel.Show(false);
			if (m_BuySellSinglePanel)
				m_BuySellSinglePanel.Show(false);
			if (m_BuyButton)
				m_BuyButton.Show(false);
			if (m_SellButton)
				m_SellButton.Show(false);
			if (m_BuyButtonSolo)
				m_BuyButtonSolo.Show(false);
			if (m_SellButtonSolo)
				m_SellButtonSolo.Show(false);
		}
		
		int resolvedLayout = 0;
		if (showDual)
			resolvedLayout = 3;
		else if (showBuySolo)
			resolvedLayout = 1;
		else if (showSellSolo)
			resolvedLayout = 2;
		m_CurrentActionLayout = resolvedLayout;
		
		Print("[AskalStore] 🔘 Botões atualizados | Mode: " + itemMode + " | Buy: " + canBuy + " | Sell: " + canSell + " | HasItem: " + playerHasItem);
	}
	
	protected void UpdateActionButtonsForBatch(bool isBuyBatch)
	{
		if (isBuyBatch)
		{
			m_CurrentSelectedClassName = "";
			UpdateActionButtons(1);
		}
		else
		{
			m_CurrentSelectedClassName = "";
			UpdateActionButtons(2);
		}
	}
	
	protected void RefreshActionButtonsForSelection()
	{
		if (m_BatchBuyEnabled || m_BatchSellEnabled)
			return;
		
		if (m_SelectedItemIndex >= 0 && m_SelectedItemIndex < m_Items.Count())
		{
			string className = m_Items.Get(m_SelectedItemIndex).GetClassName();
			if (m_SelectedVariantClass && m_SelectedVariantClass != "")
				className = m_SelectedVariantClass;
			m_CurrentSelectedClassName = className;
			int mode = ResolveItemModeForClass(className, m_SelectedItemIndex);
			UpdateActionButtons(mode);
		}
		else if (m_SelectedInventoryItem)
		{
			string invClass = m_SelectedInventoryItem.GetType();
			m_CurrentSelectedClassName = invClass;
			int invMode = ResolveItemModeForClass(invClass, -1);
			UpdateActionButtons(invMode);
			}
			else
			{
			m_CurrentSelectedClassName = "";
			UpdateActionButtons(0);
		}
	}
	
	protected bool TryGetModeFromKey(string key, out int mode)
	{
		mode = -1;
		if (!m_VirtualStoreSetupModes || !key || key == "")
			return false;
		
		int foundMode;
		if (m_VirtualStoreSetupModes.Find(key, foundMode))
		{
			mode = foundMode;
			return true;
		}
		
		string normalized = key;
		normalized.ToLower();
		if (normalized != key && m_VirtualStoreSetupModes.Find(normalized, foundMode))
		{
			mode = foundMode;
			return true;
		}
		
		string upperKey = key;
		upperKey.ToUpper();
		if (upperKey != key && m_VirtualStoreSetupModes.Find(upperKey, foundMode))
		{
			mode = foundMode;
			return true;
		}
		
		if (key.IndexOf("DS_") != 0)
		{
			string dsKey = "DS_" + key;
			if (m_VirtualStoreSetupModes.Find(dsKey, foundMode))
			{
				mode = foundMode;
				return true;
			}
		}
		
		if (key.IndexOf("CAT_") != 0)
		{
			string catKey = "CAT_" + key;
			if (m_VirtualStoreSetupModes.Find(catKey, foundMode))
			{
				mode = foundMode;
				return true;
			}
		}
		
		return false;
	}
	
	protected void ResolveDatasetAndCategoryForClass(string className, out string datasetId, out string categoryId)
	{
		datasetId = "";
		categoryId = "";
		
		// Verificar cache primeiro (performance O(1))
		if (m_ItemToDatasetCategoryCache && m_ItemToDatasetCategoryCache.Contains(className))
		{
			Param2<string, string> cached = m_ItemToDatasetCategoryCache.Get(className);
			if (cached)
			{
				datasetId = cached.param1;
				categoryId = cached.param2;
				return;
			}
		}
		
		// Se não encontrou no cache, fazer busca linear (fallback)
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (!cache)
			return;
		
		map<string, ref AskalDatasetSyncData> datasets = cache.GetDatasets();
		if (!datasets)
			return;
		
		string normalizedClass = NormalizeClassName(className);
		
		for (int i = 0; i < datasets.Count(); i++)
		{
			AskalDatasetSyncData dataset = datasets.GetElement(i);
			if (!dataset || !dataset.Categories)
				continue;
			
			string dsId = dataset.DatasetID;
			for (int j = 0; j < dataset.Categories.Count(); j++)
			{
				AskalCategorySyncData category = dataset.Categories.GetElement(j);
				if (!category || !category.Items)
					continue;
				
				if (category.Items.Contains(className) || (normalizedClass != className && category.Items.Contains(normalizedClass)))
				{
					datasetId = dsId;
					categoryId = category.CategoryID;
					
					// Adicionar ao cache para próximas buscas
					if (m_ItemToDatasetCategoryCache)
					{
						m_ItemToDatasetCategoryCache.Set(className, new Param2<string, string>(datasetId, categoryId));
					}
					
					return;
				}
			}
		}
	}
	
	protected int ResolveItemModeForClass(string className, int itemIndex = -1)
	{
		if (!className || className == "")
			return 3;
		
		// Declarar variáveis uma única vez no início da função
		string datasetId = "";
		string categoryId = "";
		
		// PRIORIDADE 1: Usar SetupItems do trader atual (se houver)
		if (m_TraderSetupItems && m_TraderSetupItems.Count() > 0)
		{
			// Tentar obter dataset e categoria do item
			if (itemIndex >= 0)
			{
				if (m_ItemDatasetIds && itemIndex < m_ItemDatasetIds.Count())
					datasetId = m_ItemDatasetIds.Get(itemIndex);
				if (m_ItemCategoryIds && itemIndex < m_ItemCategoryIds.Count())
					categoryId = m_ItemCategoryIds.Get(itemIndex);
			}
			
			// Se não encontrou, tentar resolver
			if (datasetId == "" || categoryId == "")
				ResolveDatasetAndCategoryForClass(className, datasetId, categoryId);
			
			// Usar GetItemMode() que respeita a hierarquia correta
			int traderMode = GetItemMode(datasetId, categoryId, className);
			if (traderMode >= 0) // Se encontrou configuração (incluindo modo 0)
				return traderMode;
			
			// Se não encontrou no trader, retornar -1 (disabled) para itens não configurados
			// Mas se "ALL" está definido, já foi considerado em GetItemMode()
			return -1;
		}
		
		// FALLBACK: Sistema antigo de VirtualStore (se não há trader configurado)
		if (!m_VirtualStoreSetupModes || m_VirtualStoreSetupModes.Count() == 0)
			return 3;
		
		int mode;
		if (TryGetModeFromKey(className, mode))
			return mode;
		
		string normalized = NormalizeClassName(className);
		if (normalized != className && TryGetModeFromKey(normalized, mode))
			return mode;
		if (itemIndex >= 0)
		{
			if (m_ItemDatasetIds && itemIndex < m_ItemDatasetIds.Count())
				datasetId = m_ItemDatasetIds.Get(itemIndex);
			if (m_ItemCategoryIds && itemIndex < m_ItemCategoryIds.Count())
				categoryId = m_ItemCategoryIds.Get(itemIndex);
		}
		
		if (datasetId == "" || categoryId == "")
			ResolveDatasetAndCategoryForClass(className, datasetId, categoryId);
		
		if (TryGetModeFromKey(categoryId, mode))
			return mode;
		
		if (TryGetModeFromKey(datasetId, mode))
			return mode;
		
		return 3;
	}
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		
		EnsureVirtualStoreConfigApplied();
		UpdateCooldownProgress();
		
		// Limpar notificações antigas automaticamente
		CleanupOldNotifications();
		
		// Verificar notificações pendentes do helper (3_Game)
		ProcessPendingNotifications();
		
		// Verificar health pendente do helper (3_Game)
		ProcessPendingHealth();
		
		// Verificar se há solicitação de abertura de menu do trader
		string pendingTraderMenu = AskalNotificationHelper.GetPendingTraderMenu();
		if (pendingTraderMenu && pendingTraderMenu != "")
		{
			// Se o menu já está aberto (esta instância), apenas log
			if (s_Instance && s_Instance == this)
			{
				Print("[AskalStore] ✅ Menu já está aberto para trader: " + pendingTraderMenu);
			}
			else
			{
				// Criar nova instância e mostrar via UIManager
				AskalStoreMenu newMenu = new AskalStoreMenu();
				if (newMenu)
				{
					GetGame().GetUIManager().ShowScriptedMenu(newMenu, NULL);
					Print("[AskalStore] ✅ Menu do trader aberto: " + pendingTraderMenu);
				}
			}
			AskalNotificationHelper.ClearPendingTraderMenu();
		}
		
		AnimateNotificationSlides();
		
		// Verificar sincronização de dados (padrão TraderX)
		// Se é cliente multiplayer e ainda não tem dados, verificar periodicamente
		if (GetGame().IsMultiplayer() && GetGame().IsClient())
		{
			m_LastSyncCheck += timeslice;
			if (m_LastSyncCheck >= SYNC_CHECK_INTERVAL)
			{
				m_LastSyncCheck = 0.0;
				
				// Backup: Se não solicitou ainda e ainda não está sincronizado, solicitar novamente
				if (!m_HasRequestedDatasets && !AskalDatabaseSync.IsClientSynced())
				{
					Print("[AskalStore] 📤 [Update] Solicitando datasets ao servidor via RPC (backup)...");
					
					// Solicitar datasets (sem parâmetros - servidor envia tudo)
					GetRPCManager().SendRPC("AskalCoreModule", "RequestDatasets", NULL, true, NULL, NULL);
					
					m_HasRequestedDatasets = true;
					Print("[AskalStore] ✅ [Update] RPC RequestDatasets enviado!");
				}
				// Se os dados chegaram e ainda não carregamos
				else if (AskalDatabaseSync.IsClientSynced() && m_Datasets.Count() == 0)
				{
					Print("[AskalStore] ✅ Dados sincronizados! Recarregando datasets...");
					LoadDatasetsFromCore();
					
					// Se temos datasets agora, carregar o primeiro
					if (m_Datasets.Count() > 0)
					{
						LoadDataset(0);
					}
				}
			}
		}
		
		// Rotação automática do preview
		if (m_SelectedItemPreview)
		{
			m_PreviewRotation += PREVIEW_ROTATION_SPEED * timeslice;
			if (m_PreviewRotation >= 360.0)
				m_PreviewRotation -= 360.0;
			
			m_SelectedItemPreview.SetModelOrientation(Vector(0, m_PreviewRotation, 0));
			
			vector finalPos = Vector(m_PreviewPosition[0], m_PreviewPosition[1], m_PreviewZoom);
			m_SelectedItemPreview.SetModelPosition(finalPos);
		}
	}
	
	// ========================================
	// HELPERS
	// ========================================

	protected string ResolveItemDisplayName(string className, string fallbackDisplayName = "")
	{
		// Se o fallback é vazio ou igual ao className, tentar obter do config
		// (isso garante que magazines/ammo sempre tentem obter DisplayName do config)
		if (!fallbackDisplayName || fallbackDisplayName == "" || fallbackDisplayName == className)
		{
			// Continuar para tentar obter do config
		}
		else
		{
			// Se o fallback é válido e diferente do className, usar ele
			return fallbackDisplayName;
		}
		
		string resolved = "";
		
		// Tentar obter displayName de várias configurações (ordem de prioridade)
		GetGame().ConfigGetText("CfgVehicles " + className + " displayName", resolved);
		if (resolved && resolved != "")
			return resolved;
		
		GetGame().ConfigGetText("CfgWeapons " + className + " displayName", resolved);
		if (resolved && resolved != "")
			return resolved;
		
		// Para munições e carregadores, tentar CfgMagazines primeiro
		GetGame().ConfigGetText("CfgMagazines " + className + " displayName", resolved);
		if (resolved && resolved != "")
		{
			// Remover prefixos de tradução se existirem
			if (resolved.IndexOf("$STR_") == 0)
			{
				string translatedMag = Widget.TranslateString(resolved);
				if (translatedMag && translatedMag != "")
					return translatedMag;
			}
			return resolved;
		}
		
		GetGame().ConfigGetText("CfgAmmo " + className + " displayName", resolved);
		if (resolved && resolved != "")
		{
			// Remover prefixos de tradução se existirem
			if (resolved.IndexOf("$STR_") == 0)
			{
				string translatedAmmo = Widget.TranslateString(resolved);
				if (translatedAmmo && translatedAmmo != "")
					return translatedAmmo;
			}
			return resolved;
		}
		
		GetGame().ConfigGetText("CfgNonAIVehicles " + className + " displayName", resolved);
		if (resolved && resolved != "")
			return resolved;
		
		// Fallback: tentar usar GetDisplayName do item se disponível
		EntityAI tempItem = EntityAI.Cast(SpawnTemporaryObject(className));
		if (tempItem)
		{
			ItemBase itemBase = ItemBase.Cast(tempItem);
			if (itemBase)
			{
				string itemDisplayName = itemBase.GetDisplayName();
				if (itemDisplayName && itemDisplayName != "")
				{
					GetGame().ObjectDelete(tempItem);
					return itemDisplayName;
				}
			}
			GetGame().ObjectDelete(tempItem);
		}
		
		return className;
	}

	protected array<string> BuildAttachmentList(AskalItemSyncData syncItem, AskalDatabaseClientCache cache, string className)
	{
		array<string> attachments = new array<string>();
		if (syncItem && syncItem.Attachments && syncItem.Attachments.Count() > 0)
		{
			foreach (string att : syncItem.Attachments)
			{
				if (att && att != "")
					attachments.Insert(att);
			}
		}
		else if (cache)
		{
			AskalItemSyncData cachedData = cache.FindItem(className);
			if (cachedData && cachedData.Attachments)
			{
				foreach (string cachedAttachment : cachedData.Attachments)
				{
					if (cachedAttachment && cachedAttachment != "")
						attachments.Insert(cachedAttachment);
				}
			}
		}
		return attachments;
	}

	protected int GetAttachmentPriceFromCache(string attachmentClass, AskalDatabaseClientCache cache)
	{
		if (!attachmentClass || attachmentClass == "")
			return 0;
		if (cache)
		{
			AskalItemSyncData attachmentData = cache.FindItem(attachmentClass);
			if (attachmentData)
				return NormalizeBuyPrice(attachmentData.BasePrice);
		}
		return 0;
	}

	protected int ComputeTotalItemPrice(int basePrice, array<string> attachments, AskalDatabaseClientCache cache)
	{
		int totalPrice = basePrice;
		if (attachments)
		{
			foreach (string attachmentClass : attachments)
			{
				int attachmentPrice = GetAttachmentPriceFromCache(attachmentClass, cache);
				if (attachmentPrice > 0)
					totalPrice += attachmentPrice;
			}
		}
		return totalPrice;
	}

	protected void ApplyDefaultAttachmentsToEntity(EntityAI entity, array<string> attachments)
	{
		if (!entity || !attachments)
			return;
		
		// Separar magazines de outros attachments (magazines devem ser aplicados por último)
		array<string> magazines = new array<string>();
		array<string> otherAttachments = new array<string>();
		
		foreach (string attachmentClass : attachments)
		{
			if (!attachmentClass || attachmentClass == "")
				continue;
			
			// Verificar se é um magazine usando config (mais eficiente que spawn)
			string configPath = "CfgMagazines " + attachmentClass;
			if (GetGame().ConfigIsExisting(configPath))
			{
				magazines.Insert(attachmentClass);
			}
			else
			{
				otherAttachments.Insert(attachmentClass);
			}
		}
		
		// Aplicar outros attachments primeiro
		foreach (string attachClass : otherAttachments)
		{
			EntityAI attachmentEntity = EntityAI.Cast(entity.GetInventory().CreateAttachment(attachClass));
			if (!attachmentEntity)
			{
				Print("[AskalStore] ⚠️ Falha ao anexar attachment no preview: " + attachClass);
				continue;
			}
		}
		
		// Aplicar magazines por último (ordem importa para preview)
		Weapon_Base weapon = Weapon_Base.Cast(entity);
		foreach (string magazineClass : magazines)
		{
			EntityAI magazineEntity = EntityAI.Cast(entity.GetInventory().CreateAttachment(magazineClass));
			if (!magazineEntity)
			{
				Print("[AskalStore] ⚠️ Falha ao anexar magazine no preview: " + magazineClass);
				continue;
			}
			
			Magazine mag = Magazine.Cast(magazineEntity);
			if (mag)
			{
				// Preencher o magazine para forçar atualização do proxy visual
				int ammoMax = mag.GetAmmoMax();
				if (ammoMax > 0)
				{
					// Preencher com quantidade máxima para garantir que o proxy apareça
					mag.LocalSetAmmoCount(ammoMax);
				}
				
				mag.Update();
			}
		}
		
		// Atualizar FSM e sincronizar estado da arma (como SpawnAttachedMagazine faz)
		if (weapon)
		{
			weapon.RandomizeFSMState();
			weapon.Synchronize();
			weapon.ShowMagazine();
		}
		
		// Forçar refresh visual após aplicar attachments
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(ForceEntityPreviewRefresh, 150, false, entity);
	}
	
	// Força refresh do preview após aplicar attachments
	protected void ForceEntityPreviewRefresh(EntityAI entity)
	{
		if (!entity)
			return;
		
		Weapon_Base weapon = Weapon_Base.Cast(entity);
		if (weapon)
		{
			weapon.ForceSyncSelectionState();
			weapon.ShowMagazine();
			weapon.Update();
		}
		else
		{
			entity.Update();
		}
	}

	protected void AddItemEntryForCategory(string datasetID, string categoryID, AskalCategorySyncData category, string className, AskalItemSyncData syncItem, map<string, bool> processedClasses, AskalDatabaseClientCache cache, bool includeVariants = true, bool createCard = true)
	{
		if (!className || processedClasses.Contains(className))
			return;
		AskalItemSyncData effectiveSync = syncItem;
		if (!effectiveSync && cache)
		{
			effectiveSync = cache.FindItem(className);
		}
		if (!effectiveSync)
		{
			effectiveSync = new AskalItemSyncData();
			effectiveSync.ClassName = className;
			if (category)
				effectiveSync.BasePrice = category.BasePrice;
			else
				effectiveSync.BasePrice = 0;
		}

		int categorySellPercent = DEFAULT_HARDCODED_SELL_PERCENT;
		if (effectiveSync.SellPercent > 0)
			categorySellPercent = NormalizeSellPercent(effectiveSync.SellPercent);
		else if (cache)
		{
			AskalItemSyncData cachedSell = cache.FindItem(className);
			if (cachedSell && cachedSell.SellPercent > 0)
				categorySellPercent = NormalizeSellPercent(cachedSell.SellPercent);
		}
		
		int categoryBaseFallback = DEFAULT_HARDCODED_BUY_PRICE;
		if (category && category.BasePrice > 0)
			categoryBaseFallback = category.BasePrice;
		
		int basePrice = NormalizeBuyPrice(effectiveSync.BasePrice, categoryBaseFallback);
		effectiveSync.BasePrice = basePrice;
		effectiveSync.SellPercent = NormalizeSellPercent(effectiveSync.SellPercent, categorySellPercent);

		if (createCard)
		{
			array<string> attachments = BuildAttachmentList(effectiveSync, cache, className);
			int totalPrice = ComputeTotalItemPrice(basePrice, attachments, cache);
			totalPrice = NormalizeBuyPrice(totalPrice, basePrice);
			int finalPrice = ApplyBuyCoefficient(totalPrice);

			AskalItemData itemData = new AskalItemData();
			itemData.SetClassName(className);
			itemData.SetDisplayName(ResolveItemDisplayName(className, effectiveSync.DisplayName));
			itemData.SetPrice(finalPrice);
			itemData.SetBasePrice(basePrice);
			itemData.SetDefaultAttachments(attachments);
			if (includeVariants && effectiveSync.Variants && effectiveSync.Variants.Count() > 0)
			{
				foreach (string variantClass : effectiveSync.Variants)
				{
					if (variantClass && variantClass != "")
						itemData.GetVariants().Insert(variantClass);
				}
			}
			m_Items.Insert(itemData);
			
			string effectiveDatasetId = datasetID;
			if (!effectiveDatasetId || effectiveDatasetId == "")
			{
				if (m_Datasets && m_CurrentDatasetIndex >= 0 && m_CurrentDatasetIndex < m_Datasets.Count())
					effectiveDatasetId = m_Datasets.Get(m_CurrentDatasetIndex);
				else
					effectiveDatasetId = "";
			}
			
			string effectiveCategoryId = categoryID;
			if ((!effectiveCategoryId || effectiveCategoryId == "") && category)
				effectiveCategoryId = category.CategoryID;
			
			if (!m_ItemDatasetIds)
				m_ItemDatasetIds = new array<string>();
			if (!m_ItemCategoryIds)
				m_ItemCategoryIds = new array<string>();
			
			m_ItemDatasetIds.Insert(effectiveDatasetId);
			m_ItemCategoryIds.Insert(effectiveCategoryId);
		}

		processedClasses.Set(className, true);

		if (includeVariants && effectiveSync.Variants && effectiveSync.Variants.Count() > 0)
		{
			foreach (string variantClassName : effectiveSync.Variants)
			{
				if (!variantClassName || processedClasses.Contains(variantClassName))
					continue;
				AskalItemSyncData variantSync = cache.FindItem(variantClassName);
				if (!variantSync)
				{
					variantSync = new AskalItemSyncData();
					variantSync.ClassName = variantClassName;
					if (category)
						variantSync.BasePrice = category.BasePrice;
					else
						variantSync.BasePrice = basePrice;
				}
				variantSync.SellPercent = NormalizeSellPercent(variantSync.SellPercent, categorySellPercent);
				AddItemEntryForCategory(datasetID, categoryID, category, variantClassName, variantSync, processedClasses, cache, false, false);
			}
		}
	}

	void AnimateNotificationSlides()
	{
		if (!m_NotificationSlidePanels || !m_NotificationAnimationStartTimes)
			return;
		
		float currentTime = GetGame().GetTickTime();
		for (int i = 0; i < m_NotificationSlidePanels.Count(); i++)
		{
			Widget slidePanel = m_NotificationSlidePanels.Get(i);
			if (!slidePanel)
				continue;
			
			float startTime = 0;
			if (i < m_NotificationAnimationStartTimes.Count())
				startTime = m_NotificationAnimationStartTimes.Get(i);
			
			if (startTime < 0)
				continue;
			
			float basePosY = 0;
			if (m_NotificationBasePosY && i < m_NotificationBasePosY.Count())
				basePosY = m_NotificationBasePosY.Get(i);
			
			float parentWidthPx = 70.0;
			if (m_NotificationSlideParentWidths && i < m_NotificationSlideParentWidths.Count())
				parentWidthPx = m_NotificationSlideParentWidths.Get(i);
			if (parentWidthPx <= 0)
				parentWidthPx = 70.0;
			
			float elapsed = currentTime - startTime;
			if (elapsed >= NOTIFICATION_ANIMATION_DURATION)
			{
				slidePanel.SetPos(0, basePosY);
				slidePanel.Update();
				m_NotificationAnimationStartTimes.Set(i, -1);
				continue;
			}
			
			float t = elapsed / NOTIFICATION_ANIMATION_DURATION;
			float eased = 1.0 - Math.Pow(1.0 - t, 3.0); // ease-out cubic
			float currentOffsetPx = Math.Lerp(70.0, 0.0, eased);
			float offsetNormalized = currentOffsetPx / parentWidthPx;
			slidePanel.SetPos(offsetNormalized, basePosY);
			slidePanel.Update();
		}
	}
	
	// ========================================
	// MÉTODOS AUXILIARES - SISTEMA DE CONTEÚDO
	// ========================================
	
	/// Calcula o preço por unidade (bala) para uma classe de munição
	protected float ResolveAmmoUnitPrice(string ammoClassName)
	{
		float unitPrice = -1.0;
		
		AskalDatabaseClientCache cache = AskalDatabaseClientCache.GetInstance();
		if (cache)
		{
			AskalItemSyncData ammoData = cache.FindItem(ammoClassName);
			if (ammoData && ammoData.BasePrice > 0)
			{
				float quantityMax = 1.0;
				Object tempAmmoObj = SpawnTemporaryObject(ammoClassName);
				ItemBase ammoItem = ItemBase.Cast(tempAmmoObj);
				
				if (ammoItem && ammoItem.HasQuantity())
				{
					quantityMax = ammoItem.GetQuantityMax();
					if (quantityMax <= 0)
						quantityMax = ammoItem.GetQuantity();
					if (quantityMax <= 0)
						quantityMax = 1.0;
				}
				
				if (tempAmmoObj)
					GetGame().ObjectDelete(tempAmmoObj);
				
				unitPrice = ammoData.BasePrice / Math.Max(quantityMax, 1.0);
			}
		}
		
		return unitPrice;
	}
	
	/// Obtém o preço por mL de um tipo de líquido
	protected float ResolveLiquidUnitPrice(int liquidType)
	{
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		return marketConfig.GetLiquidPricePerML(liquidType);
	}
	
	/// Obtém todos os tipos de munição compatíveis com um Magazine
	protected void GetCompatibleAmmoTypes(string magazineClass, out array<string> ammoTypes)
	{
		ammoTypes.Clear();
		
		// Lê ammoItems[] do config
		TStringArray configAmmoItems = new TStringArray();
		GetGame().ConfigGetTextArray("CfgMagazines " + magazineClass + " ammoItems", configAmmoItems);
		
		if (configAmmoItems && configAmmoItems.Count() > 0)
		{
			foreach (string ammoItem : configAmmoItems)
			{
				if (ammoItem && ammoItem != "")
					ammoTypes.Insert(ammoItem);
			}
		}
		
		// Fallback: usa o ammo padrão se não houver ammoItems[]
		if (ammoTypes.Count() == 0)
		{
			string defaultAmmo = GetGame().ConfigGetTextOut("CfgMagazines " + magazineClass + " ammo");
			if (defaultAmmo && defaultAmmo != "")
			{
				// Tenta encontrar o ammo item correspondente
				string ammoItemName = "Ammo_" + defaultAmmo.Substring(7, defaultAmmo.Length() - 7); // Remove "Bullet_"
				ammoTypes.Insert(ammoItemName);
			}
		}
		
		Print("[AskalStore] 🔫 Magazine " + magazineClass + " aceita " + ammoTypes.Count() + " tipos de munição");
	}
	
	/// Obtém todos os tipos de líquidos compatíveis com um Bottle_Base
	protected void GetCompatibleLiquidTypes(string bottleClass, out array<int> liquidTypes)
	{
		liquidTypes.Clear();
		
		// Cria objeto temporário para testar compatibilidade
		Object tempObj = SpawnTemporaryObject(bottleClass);
		if (!tempObj)
		return;
	
		ItemBase bottle = ItemBase.Cast(tempObj);
		if (!bottle)
	{
			GetGame().ObjectDelete(tempObj);
		return;
	}
	
	// Obtém o liquidType padrão
	int defaultLiquidType = bottle.GetLiquidTypeInit();
	
	// Como o DayZ não expõe CanReceiveLiquid na API pública,
	// (a maioria dos Bottle_Base aceita múltiplos tipos de líquido)
	AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
	array<int> allLiquidTypes = new array<int>();
	marketConfig.GetAllLiquidTypes(allLiquidTypes);
	
	// Adiciona todos os líquidos conhecidos
	foreach (int liquidType : allLiquidTypes)
	{
		liquidTypes.Insert(liquidType);
	}
	
	// Garante que o tipo padrão está sempre incluído primeiro
	if (defaultLiquidType > 0)
	{
		int defaultIdx = liquidTypes.Find(defaultLiquidType);
		if (defaultIdx > 0)
		{
			// Move o default para a primeira posição
			liquidTypes.Remove(defaultIdx);
			liquidTypes.InsertAt(defaultLiquidType, 0);
		}
		else if (defaultIdx == -1)
		{
			// Adiciona no início se não estava na lista
			liquidTypes.InsertAt(defaultLiquidType, 0);
		}
	}
		
		GetGame().ObjectDelete(tempObj);
		
		Print("[AskalStore] 💧 Bottle_Base " + bottleClass + " aceita " + liquidTypes.Count() + " tipos de líquido");
	}
	
	/// Obtém o nome de exibição de um tipo de munição
	protected string GetAmmoDisplayName(string ammoClassName)
	{
		string displayName = "";
		GetGame().ConfigGetText("CfgMagazines " + ammoClassName + " displayName", displayName);
		
		if (displayName && displayName != "")
		{
			// Remove prefixos de tradução se existirem
			if (displayName.IndexOf("$STR_") == 0)
				displayName = Widget.TranslateString(displayName);
			
			return displayName;
		}
		
		// Fallback: usa o nome da classe sem prefixo
		if (ammoClassName.IndexOf("Ammo_") == 0)
			return ammoClassName.Substring(5, ammoClassName.Length() - 5);
		
		return ammoClassName;
	}
	
	/// Obtém o nome de exibição de um tipo de líquido
	protected string GetLiquidDisplayName(int liquidType)
	{
		AskalMarketConfig marketConfig = AskalMarketConfig.GetInstance();
		return marketConfig.GetLiquidName(liquidType);
	}
	
	// ========================================
	// SISTEMA DE INVENTÁRIO
	// ========================================
	
	/// Escaneia o inventário do player e armazena os itens por className
	protected void ScanPlayerInventory()
	{
		Print("[AskalStore] 🔍 Escaneando inventário do player...");
		
		m_PlayerInventoryItems.Clear();
		m_InventoryScanned = true;
		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
		{
			Print("[AskalStore] ❌ Player não encontrado!");
			return;
		}
		
		// Scan all inventory items (INCLUINDO itens dentro de containers - busca recursiva)
		array<EntityAI> inventoryItems = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, inventoryItems);
		
		Print("[AskalStore] 📦 Total de itens no inventário: " + inventoryItems.Count());
		
		int totalUniqueTypes = 0;
		foreach (EntityAI item : inventoryItems)
		{
			if (!item)
				continue;
			
			// Itens com cargo serão bloqueados na venda, mas devem aparecer na listagem
			string className = item.GetType();
			string normalizedClass = NormalizeClassName(className);
			
			// Add to map (usando className normalizado)
			if (!m_PlayerInventoryItems.Contains(normalizedClass))
			{
				m_PlayerInventoryItems.Insert(normalizedClass, new array<EntityAI>());
				totalUniqueTypes++;
			}
			
			m_PlayerInventoryItems.Get(normalizedClass).Insert(item);
		}
		
		Print("[AskalStore] ✅ Inventário escaneado: " + totalUniqueTypes + " tipos de itens únicos");
		
		// Solicitar health dos itens do servidor (se multiplayer)
		if (GetGame().IsMultiplayer() && GetGame().IsClient())
		{
			RequestInventoryHealth();
		}
		
		// Processar health pendente do helper (3_Game)
		ProcessPendingHealth();
	}
	
	/// Solicita health dos itens do inventário ao servidor
	protected void RequestInventoryHealth()
	{
		if (!GetGame().IsClient())
			return;
		
		Print("[AskalStore] 📤 Solicitando health dos itens do inventário ao servidor...");
		GetRPCManager().SendRPC("AskalCoreModule", "RequestInventoryHealth", NULL, true, NULL, NULL);
	}
	
	/// Processa health pendente do helper (3_Game)
	protected void ProcessPendingHealth()
	{
		if (!m_ItemHealthMap)
			return;
		
		array<ref Param2<string, float>> healthArray = AskalNotificationHelper.GetInventoryHealth();
		if (!healthArray || healthArray.Count() == 0)
			return;
		
		Print("[AskalStore] 📥 Processando health de " + healthArray.Count() + " itens do helper");
		
		// Limpar map anterior
		m_ItemHealthMap.Clear();
		
		// Armazenar health no map (normalizado para lowercase)
		for (int i = 0; i < healthArray.Count(); i++)
		{
			Param2<string, float> healthData = healthArray.Get(i);
			if (!healthData)
				continue;
			
			string className = healthData.param1;
			float healthPercent = healthData.param2;
			
			if (!className || className == "")
				continue;
			
			// Normalizar className para lowercase
			string normalizedClass = NormalizeClassName(className);
			
			// Armazenar no map
			m_ItemHealthMap.Insert(normalizedClass, healthPercent);
		}
		
		Print("[AskalStore] ✅ Health processado e armazenado para " + m_ItemHealthMap.Count() + " itens");
		
		// Limpar health do helper após processar
		AskalNotificationHelper.ClearInventoryHealth();
		
		// Re-renderizar se estiver mostrando inventário
		if (m_ShowingInventoryForSale || m_BatchSellEnabled)
		{
			RenderInventoryItemsForSale();
		}
	}
	
	/// Normaliza className para lowercase
	protected string NormalizeClassName(string className)
	{
		if (!className || className == "")
			return "";
		
		string normalized = className;
		normalized.ToLower();
		return normalized;
	}
	
	/// Verifica se um item está no inventário do player
	protected bool IsItemInInventory(string className)
	{
		if (!m_InventoryScanned)
			return false;
		
		string normalizedClass = NormalizeClassName(className);
		return m_PlayerInventoryItems.Contains(normalizedClass);
	}
	
	/// Retorna o primeiro item encontrado no inventário com o className especificado
	protected EntityAI GetFirstItemInInventory(string className)
	{
		if (!IsItemInInventory(className))
			return null;
		
		string normalizedClass = NormalizeClassName(className);
		array<EntityAI> items = m_PlayerInventoryItems.Get(normalizedClass);
		if (items && items.Count() > 0)
			return items.Get(0);
		
		return null;
	}
	
	// ========================================
	// SISTEMA DE COOLDOWN E INTERAÇÃO
	// ========================================
	
	override bool OnKeyDown(Widget w, int x, int y, int key)
	{
		if (super.OnKeyDown(w, x, y, key))
			return true;
		
		if (key == KeyCode.KC_C)
		{
			ButtonWidget buyButton = GetActiveBuyButton();
			if (buyButton && !IsButtonCoolingDown(buyButton))
				OnPurchaseClick(buyButton);
			return true;
		}
		
		if (key == KeyCode.KC_V)
		{
			ButtonWidget sellButton = GetActiveSellButton();
			if (sellButton && !IsButtonCoolingDown(sellButton))
				OnSellClick(sellButton);
			return true;
		}
		
		return false;
	}
	
	protected ButtonWidget GetActiveBuyButton()
	{
		if (!m_CurrentCanBuy)
			return null;
		
		if (m_BuyButton && m_BuyButton.IsVisible())
			return m_BuyButton;
		if (m_BuyButtonSolo && m_BuyButtonSolo.IsVisible())
			return m_BuyButtonSolo;
		if (m_BuyButton)
			return m_BuyButton;
		return m_BuyButtonSolo;
	}
	
	protected ButtonWidget GetActiveSellButton()
	{
		if (!m_CurrentCanSell)
			return null;
		
		if (m_SellButton && m_SellButton.IsVisible())
			return m_SellButton;
		if (m_SellButtonSolo && m_SellButtonSolo.IsVisible())
			return m_SellButtonSolo;
		if (m_SellButton)
			return m_SellButton;
		return m_SellButtonSolo;
	}
	
	protected array<ButtonWidget> CollectCooldownGroup(ButtonWidget sourceButton)
	{
		array<ButtonWidget> group = new array<ButtonWidget>();
		if (!sourceButton)
			return group;
		
		if (sourceButton == m_BuyButton || sourceButton == m_BuyButtonSolo)
		{
			if (m_BuyButton)
				group.Insert(m_BuyButton);
			if (m_BuyButtonSolo && m_BuyButtonSolo != m_BuyButton)
				group.Insert(m_BuyButtonSolo);
		}
		else if (sourceButton == m_SellButton || sourceButton == m_SellButtonSolo)
		{
			if (m_SellButton)
				group.Insert(m_SellButton);
			if (m_SellButtonSolo && m_SellButtonSolo != m_SellButton)
				group.Insert(m_SellButtonSolo);
		}
		else
		{
			group.Insert(sourceButton);
		}
		
		return group;
	}
	
	protected ProgressBarWidget GetCooldownProgressBar(ButtonWidget button)
	{
		if (!button)
			return null;
		
		if (button == m_BuyButton)
			return m_BuyCooldownProgressBar;
		if (button == m_BuyButtonSolo)
			return m_BuySoloCooldownProgressBar;
		if (button == m_SellButton)
			return m_SellCooldownProgressBar;
		if (button == m_SellButtonSolo)
			return m_SellSoloCooldownProgressBar;
		
		return null;
	}
	
	protected bool IsButtonCoolingDown(ButtonWidget button)
	{
		if (!button || !m_ButtonCooldownStartTimes)
			return false;
		return m_ButtonCooldownStartTimes.Contains(button);
	}
	
	protected void StartButtonCooldown(ButtonWidget sourceButton)
	{
		if (!sourceButton || m_CooldownDuration <= 0)
			return;
		
		float currentTime = GetGame().GetTickTime();
		array<ButtonWidget> group = CollectCooldownGroup(sourceButton);
		
		for (int i = 0; i < group.Count(); i++)
		{
			ButtonWidget button = group.Get(i);
			if (!button)
				continue;
			
			button.Enable(false);
			m_ButtonCooldownStartTimes.Set(button, currentTime);
			
			ProgressBarWidget pb = GetCooldownProgressBar(button);
			if (pb)
			{
				pb.SetCurrent(0);
				pb.Show(true);
			}
		}
		
		Print("[AskalStore] ⏱️ Cooldown iniciado: " + m_CooldownDuration + "s");
	}
	
	protected void UpdateCooldownProgress()
	{
		if (!m_ButtonCooldownStartTimes || m_ButtonCooldownStartTimes.Count() == 0)
			return;
		
		float currentTime = GetGame().GetTickTime();
		
		for (int i = m_ButtonCooldownStartTimes.Count() - 1; i >= 0; i--)
		{
			ButtonWidget button = m_ButtonCooldownStartTimes.GetKey(i);
			float startTime = m_ButtonCooldownStartTimes.GetElement(i);
			float elapsed = currentTime - startTime;
			
			float progressNormalized = 1.0;
			if (m_CooldownDuration > 0)
				progressNormalized = Math.Clamp(elapsed / m_CooldownDuration, 0.0, 1.0);
			
			ProgressBarWidget pb = GetCooldownProgressBar(button);
			if (pb)
			{
				pb.SetCurrent(progressNormalized * 100.0);
				pb.Show(true);
			}
			
			if (elapsed >= m_CooldownDuration || m_CooldownDuration <= 0)
			{
				if (button)
					button.Enable(true);
				if (pb)
				{
					pb.SetCurrent(0);
					pb.Show(false);
				}
				m_ButtonCooldownStartTimes.Remove(button);
			}
		}
	}
	
	protected void ResetAllCooldowns()
	{
		if (!m_ButtonCooldownStartTimes)
			return;
		
		for (int i = m_ButtonCooldownStartTimes.Count() - 1; i >= 0; i--)
		{
			ButtonWidget button = m_ButtonCooldownStartTimes.GetKey(i);
			if (button)
				button.Enable(true);
			
			ProgressBarWidget pb = GetCooldownProgressBar(button);
			if (pb)
			{
				pb.SetCurrent(0);
				pb.Show(false);
			}
		}
		
		m_ButtonCooldownStartTimes.Clear();
	}
	
	protected bool ProcessBatchSell(ButtonWidget sourceButton = null)
	{
		if (!m_BatchSellSelectedEntities || m_BatchSellSelectedEntities.Count() == 0)
		{
			// Se não há itens selecionados para lote, tentar venda única
			return ProcessSingleSell(sourceButton);
		}
		
		int successCount = 0;
		int totalItems = m_BatchSellSelectedEntities.Count();
		
		for (int idx = 0; idx < totalItems; idx++)
		{
			EntityAI item = m_BatchSellSelectedEntities.Get(idx);
			if (!item)
			{
				continue;
			}
			
			if (SendSellRequest(item))
			{
				successCount++;
			}
			else
			{
			}
		}
		
		if (successCount == 0)
		{
			DisplayTransactionError("Nenhum item válido para venda em lote");
			return false;
		}
		
		
		Print("[AskalStore] 📤 Venda em lote solicitada para " + successCount + " itens");
		
		ClearBatchSelections(true);
		m_SelectedInventoryItem = null;
		ScanPlayerInventory();
		RenderInventoryItemsForSale();
		UpdateTransactionSummary();
		DisplayTransactionMessage("Impossível Vender");
		return true;
	}
	
	// Método público para lidar com resposta de venda do servidor
	void OnSellResponse(bool success, string message, string itemClass, int price)
	{
		if (success)
		{
			ShowNotification("✅ " + message, "#00FF00");
			// Notificação visual já é adicionada por SellItemResponse em askalcoremodule.c
			// Não precisamos adicionar aqui para evitar duplicação
			
			// Atualizar inventário após venda bem-sucedida
			ScanPlayerInventory();
			if (m_BatchSellEnabled || m_ShowingInventoryForSale)
				RenderInventoryItemsForSale();
			UpdateTransactionSummary();
		}
		else
		{
			ShowNotification("❌ " + message, "#FF0000");
			
			// Mensagem específica para itens ocupados
			if (message.IndexOf("ocupado") != -1 || message.IndexOf("esvazie") != -1)
			{
				DisplayTransactionError("[AVISO] Item ocupado: esvazie o inventario do item antes de vende-lo");
			}
			else
			{
				DisplayTransactionError(message);
			}
		}
	}
	
	
	// Obter health de um item (retorna 100% se não encontrado)
	protected float GetItemHealth(string className)
	{
		if (!m_ItemHealthMap || !className || className == "")
			return 100.0;
		
		string normalizedClass = NormalizeClassName(className);
		if (m_ItemHealthMap.Contains(normalizedClass))
		{
			return m_ItemHealthMap.Get(normalizedClass);
		}
		
		// Fallback: 100% se não encontrado
		return 100.0;
	}
}

