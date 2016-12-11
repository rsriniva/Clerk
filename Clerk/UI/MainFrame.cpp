#include "MainFrame.h"

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	Settings::GetInstance().Open("Config.txt");

	this->SetSize(wxSize(Settings::GetInstance().GetWindowWidth(), Settings::GetInstance().GetWindowHeight()));
	selectedAccountId = Settings::GetInstance().GetSelectedAccountId();

	wxImage image;

	accountsImageList = new wxImageList(16, 16, true);

	for (int i = 0; i <= 50; i++) {
		wxString path = wxString::Format("Resources\\Accounts Icons\\%d.png", i);
		if (image.LoadFile(path, wxBITMAP_TYPE_PNG))
		{
			wxBitmap *bitmap = new wxBitmap(image);
			accountsImageList->Add(*bitmap);

			delete bitmap;
		}
	}

	for (int i = 0; i <= 19; i++) {
		wxString path = wxString::Format("Resources\\%d.png", i);
		if (image.LoadFile(path, wxBITMAP_TYPE_PNG))
		{
			wxBitmap *bitmap = new wxBitmap(image);
			accountsImageList->Add(*bitmap);

			delete bitmap;
		}
	}

	wxMenu *menuFile = new wxMenu();

	menuFile->Append(wxID_ABOUT, "&About...");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, "E&xit\tCtrl+W");

	wxMenu *menuAccounts = new wxMenu();
	menuAccounts->Append(ID_ADD_ACCOUNT, wxT("Add Account...Ctrl+N"));

	wxMenu *menuTransactions = new wxMenu();
	menuTransactions->Append(ID_ADD_TRANSACTION, wxT("Add...\tCtrl+T"));
	menuTransactions->Append(ID_DUPLICATE_TRANSACTION, wxT("Duplicate...\tCtrl+D"));
	menuTransactions->Append(ID_SPLIT_TRANSACTION, wxT("Split...\tCtrl+S"));

	wxMenuBar *menuBar = new wxMenuBar();

	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuAccounts, "&Accounts");
	menuBar->Append(menuTransactions, "&Transactions");

	SetMenuBar(menuBar);

	wxToolBar *toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_TEXT | wxTB_HORZ_TEXT | wxBORDER_NONE);
	toolbar->AddTool(ID_ADD_TRANSACTION, wxT("Add Transaction"), accountsImageList->GetBitmap(62), wxT("Add Transaction"));	
	toolbar->Realize();

	SetToolBar(toolbar);

	wxSplitterWindow *splittermain = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER);
	splittermain->SetSashGravity(0.5);
	splittermain->SetMinimumPaneSize(300);

	wxPanel *panel4 = new wxPanel(splittermain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	panel4->SetBackgroundColour(wxColour(245, 245, 245, 1));

	wxBoxSizer *boxSizer4 = new wxBoxSizer(wxVERTICAL);

	treeMenu = new wxTreeCtrl(panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS | wxBORDER_NONE);
	treeMenu->SetBackgroundColour(wxColour(245, 245, 245, 1));
	treeMenu->SetForegroundColour(wxColour(68, 68, 68, 1));

	treeMenu->AssignImageList(accountsImageList);
	treeMenu->AddRoot("Accounts", -1, -1, 0);

	boxSizer4->Add(treeMenu, 1, wxEXPAND | wxALL, 0);
	panel4->SetSizer(boxSizer4);

	CreateStatusBar();
	SetStatusText("");

	wxPanel *panel = new wxPanel(splittermain, wxID_ANY);

	panel2 = new wxPanel(panel, wxID_ANY);
	panel3 = new wxPanel(panel, wxID_ANY);
	panel5 = new wxPanel(panel, wxID_ANY);

	transactionList = new TransactionsListPanel(panel2, wxID_ANY);
	transactionList->OnEditTransaction = std::bind(&MainFrame::EditTransaction, this);
	transactionList->OnSplitTransaction = std::bind(&MainFrame::SplitTransaction, this);
	transactionList->OnPeriodChanged = std::bind(&MainFrame::UpdateStatus, this);

	homePanel = new HomePanel(panel3, wxID_ANY);

	wxBoxSizer *boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(transactionList, 1, wxEXPAND | wxALL, 0);
	panel2->SetSizer(boxSizer);	

	boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(homePanel, 1, wxEXPAND | wxALL, 0);
	panel3->SetSizer(boxSizer);

	reportPanel = new ReportPanel(panel5, wxID_ANY);

	boxSizer = new wxBoxSizer(wxVERTICAL);
	boxSizer->Add(reportPanel, 1, wxEXPAND | wxALL, 0);
	panel5->SetSizer(boxSizer);

	vbox = new wxBoxSizer(wxVERTICAL);

	panel->SetSizer(vbox);

	splittermain->SplitVertically(panel4, panel, 1);

	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAbout, this, wxID_ABOUT);
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnQuit, this, wxID_EXIT);

	menuAccounts->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddAccount, this, ID_ADD_ACCOUNT);

	menuTransactions->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddTransaction, this, ID_ADD_TRANSACTION);
	menuTransactions->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnDuplicateTransaction, this, ID_DUPLICATE_TRANSACTION);
	menuTransactions->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnSplitTransaction, this, ID_SPLIT_TRANSACTION);

	toolbar->Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnAddTransaction, this, ID_ADD_TRANSACTION);

	treeMenu->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::OnTreeItemSelect, this);
	treeMenu->Bind(wxEVT_TREE_ITEM_MENU, &MainFrame::OnTreeSpecItemMenu, this);

	UpdateAccountsTree();
}

MainFrame::~MainFrame() 
{
	Settings::GetInstance().SetSelectedAccountId(selectedAccountId);
	Settings::GetInstance().SetWindowWidth(this->GetSize().GetWidth());
	Settings::GetInstance().SetWindowHeight(this->GetSize().GetHeight());

	Settings::GetInstance().Save();
	
	delete treeMenu;
	delete transactionList;
	delete homePanel;
	delete reportPanel;
}

void MainFrame::UpdateAccountsTree() 
{
	wxTreeItemId rootItem = treeMenu->GetRootItem();

	accounts.clear();
	reports.clear();

	treeMenu->DeleteChildren(rootItem);

	TreeMenuItemData *itemData = new TreeMenuItemData();
	itemData->type = TreeMenuItemTypes::MenuHome;

	wxTreeItemId homeItem = treeMenu->AppendItem(rootItem, "Home", 15, 15, itemData);
	wxTreeItemId accountsItem = treeMenu->AppendItem(homeItem, "Accounts", 33, 33);
	wxTreeItemId reportsItem = treeMenu->AppendItem(homeItem, "Reports", 51, 51);
	wxTreeItemId budgetsItem = treeMenu->AppendItem(homeItem, "Budgets", 57, 57);

	itemData = new TreeMenuItemData();
	itemData->type = TreeMenuItemTypes::MenuDeposits;

	wxTreeItemId child = treeMenu->AppendItem(accountsItem, "Deposits", 32, 32, itemData);
	wxTreeItemId selectedItem = homeItem;

	for each (auto account in DataHelper::GetInstance().GetAccounts(AccountTypes::Deposit))
	{
		int icon = 26;

		if (account->iconId <= accountsImageList->GetImageCount()) {
			icon = account->iconId;
		}

		TreeMenuItemData *itemData = new TreeMenuItemData();
		itemData->type = TreeMenuItemTypes::MenuAccount;
		itemData->object = account;

		wxString name = wxString::FromUTF8(account->name->c_str());

		wxTreeItemId itemId = treeMenu->AppendItem(child, name, icon, icon, itemData);
		accounts.push_back(account);

		if (account->id == selectedAccountId) {
			selectedItem = itemId;
		}
	}

	itemData = new TreeMenuItemData();
	itemData->type = TreeMenuItemTypes::MenuReceipts;

	child = treeMenu->AppendItem(accountsItem, "Receipts", 32, 32, itemData);

	for each (auto account in DataHelper::GetInstance().GetAccounts(AccountTypes::Receipt))
	{
		int icon = 27;

		if (account->iconId <= accountsImageList->GetImageCount()) {
			icon = account->iconId;
		}

		TreeMenuItemData *itemData = new TreeMenuItemData();
		itemData->type = TreeMenuItemTypes::MenuAccount;
		itemData->object = account;

		wxString name = wxString::FromUTF8(account->name->c_str());

		wxTreeItemId itemId = treeMenu->AppendItem(child, name, icon, icon, itemData);
		accounts.push_back(account);

		if (account->id == selectedAccountId) {
			selectedItem = itemId;
		}
	}

	itemData = new TreeMenuItemData();
	itemData->type = TreeMenuItemTypes::MenuExpenses;

	child = treeMenu->AppendItem(accountsItem, "Expenes", 32, 32, itemData);

	for each (auto account in DataHelper::GetInstance().GetAccounts(AccountTypes::Expens))
	{
		int icon = 28;

		if (account->iconId <= accountsImageList->GetImageCount()) {
			icon = account->iconId;
		}

		TreeMenuItemData *itemData = new TreeMenuItemData();
		itemData->type = TreeMenuItemTypes::MenuAccount;
		itemData->object = account;

		wxString name = wxString::FromUTF8(account->name->c_str());

		wxTreeItemId itemId = treeMenu->AppendItem(child, name, icon, icon, itemData);
		accounts.push_back(account);

		if (account->id == selectedAccountId) {
			selectedItem = itemId;
		}
	}

	child = treeMenu->AppendItem(accountsItem, "Debt", 32, 32);

	for each (auto account in DataHelper::GetInstance().GetAccounts(AccountTypes::Debt))
	{
		int icon = 28;

		if (account->iconId <= accountsImageList->GetImageCount()) {
			icon = account->iconId;
		}

		TreeMenuItemData *itemData = new TreeMenuItemData();
		itemData->type = TreeMenuItemTypes::MenuAccount;
		itemData->object = account;

		wxString name = wxString::FromUTF8(account->name->c_str());

		wxTreeItemId itemId = treeMenu->AppendItem(child, name, icon, icon, itemData);
		accounts.push_back(account);

		if (account->id == selectedAccountId) {
			selectedItem = itemId;
		}
	}

	child = treeMenu->AppendItem(accountsItem, "Credits", 32, 32);

	for each (auto account in DataHelper::GetInstance().GetAccounts(AccountTypes::Credit))
	{
		int icon = 28;

		if (account->iconId <= accountsImageList->GetImageCount()) {
			icon = account->iconId;
		}

		TreeMenuItemData *itemData = new TreeMenuItemData();
		itemData->type = TreeMenuItemTypes::MenuAccount;
		itemData->object = account;

		wxString name = wxString::FromUTF8(account->name->c_str());

		wxTreeItemId itemId = treeMenu->AppendItem(child, name, icon, icon, itemData);
		accounts.push_back(account);

		if (account->id == selectedAccountId) {
			selectedItem = itemId;
		}
	}

	auto report = make_shared<Report>(-1);
	report->name = make_shared<wxString>("Expenses");

	reports.push_back(report);

	itemData = new TreeMenuItemData();
	itemData->type = TreeMenuItemTypes::MenuReport;	

	itemData->object = report;

	wxTreeItemId itemId = treeMenu->AppendItem(reportsItem, *report->name, 51, 51, itemData);

	treeMenu->Expand(accountsItem);

	if (selectedItem) {
		treeMenu->SelectItem(selectedItem);
	}
}

void MainFrame::UpdateTransactionList(TreeMenuItemTypes type, Account *account) 
{
	if (type == TreeMenuItemTypes::MenuAccount) {
		transactionList->SetType(type);
		transactionList->SetAccount(account);
		transactionList->Update();

		float amount = 0;

		if (account->type == AccountTypes::Deposit || account->type == AccountTypes::Credit) {
			amount = DataHelper::GetInstance().GetBalance(account);
		}
		else if (account->type == AccountTypes::Expens) {
			amount = transactionList->GetBalance();
		}
		else {
			amount = DataHelper::GetInstance().GetToAmountSum(account, &transactionList->GetFromDate(), &transactionList->GetToDate());
		}

		wxString name = wxString::FromUTF8(account->name->c_str());

		if (account->creditLimit > 0.0) {
			SetStatusText(wxString::Format("%s: %.2f (%.2f %.2f) %s", static_cast<const char*>(name), account->creditLimit + amount, account->creditLimit, amount, static_cast<const char*>(account->currency->shortName->c_str())));
		}
		else {
			SetStatusText(wxString::Format("%s: %.2f %s", static_cast<const char*>(name), amount, static_cast<const char*>(account->currency->shortName->c_str())));
		}
	}
	else if (type == TreeMenuItemTypes::MenuExpenses) {
		transactionList->SetType(type);
		transactionList->Update();

		float amount = transactionList->GetBalance();
		SetStatusText(wxString::Format("Expenses: %.2f", amount));
	}
	else if (type == TreeMenuItemTypes::MenuReceipts) {
		transactionList->SetType(type);
		transactionList->Update();

		float amount = transactionList->GetBalance();
		SetStatusText(wxString::Format("Receipts: %.2f", amount));
	}
	else if (type == TreeMenuItemTypes::MenuDeposits) {
		transactionList->SetType(type);
		transactionList->Update();

		float amount = transactionList->GetBalance();
		SetStatusText(wxString::Format("Deposits: %.2f", amount));
	}
}

void MainFrame::UpdateStatus() {
	wxTreeItemId itemId = treeMenu->GetSelection();

	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {
		if (item->type == TreeMenuItemTypes::MenuAccount) {
			std::shared_ptr<Account> account = std::static_pointer_cast<Account>(item->object);
			UpdateTransactionList(TreeMenuItemTypes::MenuAccount, account.get());
		}
		else if (item->type == TreeMenuItemTypes::MenuExpenses) {
			UpdateTransactionList(TreeMenuItemTypes::MenuExpenses, 0);
		}
		else if (item->type == TreeMenuItemTypes::MenuReceipts) {
			UpdateTransactionList(TreeMenuItemTypes::MenuReceipts, 0);
		}
		else if (item->type == TreeMenuItemTypes::MenuDeposits) {
			UpdateTransactionList(TreeMenuItemTypes::MenuDeposits, 0);
		}
	}
}

void MainFrame::OnQuit(wxCommandEvent &event)
{
	Close(TRUE);
}

void MainFrame::OnAbout(wxCommandEvent &event)
{
	wxMessageBox("Personal finance application", "About Clerk", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnTreeSpecItemMenu(wxTreeEvent &event)
{
	wxMenu *menu = new wxMenu();

	menu->Append(ID_ADD_ACCOUNT, wxT("Add Account..."));
	menu->Append(ID_EDIT_ACCOUNT, wxT("Edit Account..."));
	menu->Append(ID_DELETE_ACCOUNT, wxT("Delete Account"));

	menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddAccount, this, ID_ADD_ACCOUNT);
	menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnEditAccount, this, ID_EDIT_ACCOUNT);
	menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnDeleteAccount, this, ID_DELETE_ACCOUNT);

	PopupMenu(menu, event.GetPoint());

	delete menu;
}

void MainFrame::OnTreeItemSelect(wxTreeEvent &event)
{
	wxTreeItemId itemId = event.GetItem();
	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {		
		vbox->Detach(panel2);
		vbox->Detach(panel3);
		vbox->Detach(panel5);

		if (item->type == TreeMenuItemTypes::MenuAccount) {
			vbox->Add(panel2, 1, wxEXPAND | wxALL, 0);

			panel2->Show();
			panel3->Hide();
			panel5->Hide();

			std::shared_ptr<Account> account = std::static_pointer_cast<Account>(item->object);
			UpdateTransactionList(TreeMenuItemTypes::MenuAccount, account.get());

			selectedAccountId = account->id;
		}
		else if (item->type == TreeMenuItemTypes::MenuExpenses) {
			vbox->Add(panel2, 1, wxEXPAND | wxALL, 0);

			panel2->Show();
			panel3->Hide();
			panel5->Hide();

			UpdateTransactionList(TreeMenuItemTypes::MenuExpenses, 0);
		}
		else if (item->type == TreeMenuItemTypes::MenuReceipts) {
			vbox->Add(panel2, 1, wxEXPAND | wxALL, 0);

			panel2->Show();
			panel3->Hide();
			panel5->Hide();

			UpdateTransactionList(TreeMenuItemTypes::MenuReceipts, 0);
		}
		else if (item->type == TreeMenuItemTypes::MenuDeposits) {
			vbox->Add(panel2, 1, wxEXPAND | wxALL, 0);

			panel2->Show();
			panel3->Hide();
			panel5->Hide();

			UpdateTransactionList(TreeMenuItemTypes::MenuDeposits, 0);
		}
		else if (item->type == TreeMenuItemTypes::MenuReport) {
			vbox->Add(panel5, 1, wxEXPAND | wxALL, 0);

			panel2->Hide();
			panel3->Hide();
			panel5->Show();

			reportPanel->Update();
		}
		else {
			vbox->Add(panel3, 1, wxEXPAND | wxALL, 0);

			panel2->Hide();
			panel3->Show();

			homePanel->Update();
		}

		vbox->Layout();
	}
}

void MainFrame::OnAddTransaction(wxCommandEvent &event) {
	AddTransaction();
}

void MainFrame::OnDuplicateTransaction(wxCommandEvent &event) {
	transactionList->DublicateTransaction();
}

void MainFrame::OnSplitTransaction(wxCommandEvent &event) {
	SplitTransaction();
}

void MainFrame::AddTransaction() {
	transactionFrame = new TransactionFrame(this, wxT("Transaction"), 0, 0, 450, 350);
	
	transactionFrame->Show(true);
	transactionFrame->CenterOnParent();

	wxTreeItemId itemId = treeMenu->GetSelection();
	
	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {
		if (item->type == TreeMenuItemTypes::MenuAccount) {
			shared_ptr<Account> account = static_pointer_cast<Account>(item->object);
			auto transaction = make_shared<Transaction>();

			transactionFrame->SetTransaction(transaction);
			transactionFrame->SetAccount(account);
		}
	}

	transactionFrame->OnClose = std::bind(&MainFrame::OnTransactionClose, this);
}

void MainFrame::EditTransaction() {
	auto transaction = transactionList->GetTransaction();

	if (transaction) {
		transactionFrame = new TransactionFrame(this, wxT("Transaction"), 0, 0, 450, 350);

		transactionFrame->Show(true);
		transactionFrame->CenterOnParent();

		transactionFrame->SetTransaction(transaction);
		transactionFrame->OnClose = std::bind(&MainFrame::OnTransactionClose, this);
	}
}

void MainFrame::SplitTransaction() {
	auto transaction = transactionList->GetTransaction();

	if (transaction) {
		transactionFrame = new TransactionFrame(this, wxT("Transaction"), 0, 0, 450, 350);

		transactionFrame->Show(true);
		transactionFrame->CenterOnParent();

		transactionFrame->SetSplitTransaction(transaction);
		transactionFrame->OnClose = std::bind(&MainFrame::OnTransactionClose, this);
	}
}

void MainFrame::OnTransactionClose() {
	delete transactionFrame;

	wxTreeItemId itemId = treeMenu->GetSelection();
	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {
		if (item->type == TreeMenuItemTypes::MenuAccount) {
			std::shared_ptr<Account> account = std::static_pointer_cast<Account>(item->object);
			UpdateTransactionList(TreeMenuItemTypes::MenuAccount, account.get());
		} else if (item->type == TreeMenuItemTypes::MenuExpenses) {
			UpdateTransactionList(TreeMenuItemTypes::MenuExpenses, 0);		
		} else if (item->type == TreeMenuItemTypes::MenuReceipts) {
			UpdateTransactionList(TreeMenuItemTypes::MenuReceipts, 0);
		} else if (item->type == TreeMenuItemTypes::MenuDeposits) {
			UpdateTransactionList(TreeMenuItemTypes::MenuDeposits, 0);
		}
	}

	homePanel->Update();
}

void MainFrame::OnAddAccount(wxCommandEvent &event) {
	AddAccount();
}

void MainFrame::OnEditAccount(wxCommandEvent &event) {
	EditAccount();
}

void MainFrame::OnDeleteAccount(wxCommandEvent &event) {
	DeleteAccount();
}

void MainFrame::AddAccount() {
	accountFrame = new AccountFrame(this, wxT("Account"), 0, 0, 340, 400);

	accountFrame->Show(true);
	accountFrame->CenterOnParent();

	std::shared_ptr<Account> account = make_shared<Account>();

	account->id = -1;
	account->name = make_shared<string>("");
	account->note = make_shared<string>("");
	account->type = AccountTypes::Deposit;
	account->iconId = 0;
	account->orderId = 1000;
	account->currency = make_shared<Currency>(152);
	
	accountFrame->SetAccount(account);

	accountFrame->OnClose = std::bind(&MainFrame::OnAccountClose, this);
}

void MainFrame::EditAccount() {
	wxTreeItemId itemId = treeMenu->GetSelection();
	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {
		if (item->type == TreeMenuItemTypes::MenuAccount) {
			accountFrame = new AccountFrame(this, wxT("Account"), 0, 0, 340, 400);

			accountFrame->Show(true);
			accountFrame->CenterOnParent();

			accountFrame->OnClose = std::bind(&MainFrame::OnAccountClose, this);

			std::shared_ptr<Account> account = std::static_pointer_cast<Account>(item->object);
			accountFrame->SetAccount(account);
		}
	}
}

void MainFrame::DeleteAccount() {
	wxTreeItemId itemId = treeMenu->GetSelection();
	TreeMenuItemData *item = (TreeMenuItemData *)treeMenu->GetItemData(itemId);

	if (item != NULL) {
		if (item->type == TreeMenuItemTypes::MenuAccount) {
			std::shared_ptr<Account> account = std::static_pointer_cast<Account>(item->object);
			account->Delete();

			UpdateAccountsTree();
		}
	}
}

void MainFrame::OnAccountClose() {
	delete accountFrame;

	UpdateAccountsTree();
}