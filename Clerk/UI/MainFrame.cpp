#include "MainFrame.h"

MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	Settings::GetInstance().Open("Config.txt");	
	DataHelper::GetInstance().CreateAccountsImageList();

	this->SetSize(wxSize(Settings::GetInstance().GetWindowWidth(), Settings::GetInstance().GetWindowHeight()));
	this->SetIcon(wxICON(APP_ICON));

	wxSplitterWindow *splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_NOBORDER);
	splitter->SetSashGravity(0.5);
	splitter->SetMinimumPaneSize(300);

	wxPanel *splitterLeftPanel = new wxPanel(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	splitterLeftPanel->SetBackgroundColour(wxColour(245, 245, 245, 1));

	wxBoxSizer *boxSizer = new wxBoxSizer(wxVERTICAL);

	treeMenu = new TreeMenu(splitterLeftPanel, wxID_ANY);

	treeMenu->OnAccountSelect = std::bind(&MainFrame::OnTreeMenuAccountSelect, this, std::placeholders::_1);
	treeMenu->OnReportSelect = std::bind(&MainFrame::OnTreeMenuReportSelect, this, std::placeholders::_1);
	treeMenu->OnDashboardSelect = std::bind(&MainFrame::OnTreeMenuDashboardSelect, this);
	treeMenu->OnBudgetsSelect = std::bind(&MainFrame::OnTreeMenuBudgetsSelect, this);
	treeMenu->OnGoalsSelect = std::bind(&MainFrame::OnTreeMenuGoalsSelect, this);
	treeMenu->OnSchedulersSelect = std::bind(&MainFrame::OnTreeMenuSchedulersSelect, this);
	treeMenu->OnTrashSelect = std::bind(&MainFrame::OnTreeMenuTrashSelect, this);
	treeMenu->OnTagsSelect = std::bind(&MainFrame::OnTreeMenuTagsSelect, this);
	treeMenu->OnAccountsSelect = std::bind(&MainFrame::OnTreeMenuAccountsSelect, this, std::placeholders::_1);
	treeMenu->OnAddAccount = std::bind(&MainFrame::AddAccount, this);
	treeMenu->OnEditAccount = std::bind(&MainFrame::EditAccount, this, std::placeholders::_1);
	treeMenu->OnArchiveAccount = std::bind(&MainFrame::DeleteAccount, this, std::placeholders::_1);
	treeMenu->OnRestoreAccount = std::bind(&MainFrame::RestoreAccount, this, std::placeholders::_1);
	treeMenu->OnAddTransaction = std::bind(&MainFrame::OnTreeMenuAddTransaction, this, std::placeholders::_1);
	treeMenu->OnAddBudget = std::bind(&MainFrame::AddBudget, this);
	treeMenu->OnAddScheduler = std::bind(&MainFrame::AddScheduler, this);
	treeMenu->OnAddGoal = std::bind(&MainFrame::AddGoal, this);
	treeMenu->OnNewTab = std::bind(&MainFrame::AddTab, this, std::placeholders::_1, std::placeholders::_2);
	treeMenu->OnEmptyTrash = std::bind(&MainFrame::OnEmptyTrash, this);

	boxSizer->Add(treeMenu, 1, wxEXPAND | wxALL, 0);
	splitterLeftPanel->SetSizer(boxSizer);

	CreateMainMenu();
	CreateStatusBar();
	SetStatusText("");

	wxPanel *splitterRightPanel = new wxPanel(splitter, wxID_ANY);	

	rightPanelSizer = new wxBoxSizer(wxVERTICAL);
	splitterRightPanel->SetSizer(rightPanelSizer);

	tabsPanel = new TabsPanel(splitterRightPanel, wxID_ANY);

	tabsPanel->OnUpdateStatus = std::bind(&MainFrame::UpdateStatus, this, std::placeholders::_1);
	tabsPanel->OnAddTransaction = std::bind(&MainFrame::AddTransactionFromContextMenu, this);
	tabsPanel->OnCopyTransaction = std::bind(&MainFrame::CopyTransaction, this, std::placeholders::_1);
	tabsPanel->OnEditTransaction = std::bind(&MainFrame::EditTransaction, this, std::placeholders::_1);
	tabsPanel->OnSplitTransaction = std::bind(&MainFrame::SplitTransaction, this, std::placeholders::_1);
	tabsPanel->OnAddBudget = std::bind(&MainFrame::AddBudget, this);
	tabsPanel->OnEditBudget = std::bind(&MainFrame::EditBudget, this, std::placeholders::_1);
	tabsPanel->OnAddScheduler = std::bind(&MainFrame::AddScheduler, this);
	tabsPanel->OnEditScheduler = std::bind(&MainFrame::EditScheduler, this, std::placeholders::_1);
	tabsPanel->OnAddGoal = std::bind(&MainFrame::AddGoal, this);
	tabsPanel->OnEditGoal = std::bind(&MainFrame::EditGoal, this, std::placeholders::_1);

	rightPanelSizer->Add(tabsPanel, 1, wxEXPAND | wxALL, 0);
	rightPanelSizer->Layout();

	splitter->SplitVertically(splitterLeftPanel, splitterRightPanel, 1);

	treeMenu->Update();
	treeMenu->RestoreState();

	tabsPanel->RestoreTabs();
	tabsPanel->UpdateStatus();
	
	std::thread([=]()
	{
		std::this_thread::sleep_for(std::chrono::seconds(3));		
		this->GetEventHandler()->CallAfter(&MainFrame::CheckSchedulers);
	}).detach();
}

MainFrame::~MainFrame() 
{
	delete treeMenu;
	delete tabsPanel;

	Settings::GetInstance().SetWindowWidth(this->GetSize().GetWidth());
	Settings::GetInstance().SetWindowHeight(this->GetSize().GetHeight());

	Settings::GetInstance().Save();
}

void MainFrame::CreateMainMenu() {
	auto transactions = DataHelper::GetInstance().GetRecentTransactions();

	wxMenu *menuFile = new wxMenu();
	wxMenu *menuHelp = new wxMenu();

	menuHelp->Append(static_cast<int>(MainMenuTypes::About), "&About...");

	if (transactions.size() == 0) {
		menuFile->Append(static_cast<int>(MainMenuTypes::AddTransaction), wxT("New Transaction...\tCtrl+T"));
		menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddTransaction, this, static_cast<int>(MainMenuTypes::AddTransaction));
	}
	else {
		wxMenu *menuTransaction = new wxMenu();
		menuFile->AppendSubMenu(menuTransaction, wxT("New Transaction"));

		menuTransaction->Append(0, wxT("New Transaction...\tCtrl+T"));
		menuTransaction->AppendSeparator();

		for each (auto transaction in transactions)
		{
			menuTransaction->Append(transaction->id, wxString::Format("%s - %s", *transaction->fromAccountName, *transaction->toAccountName));
		}

		menuTransaction->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OnAddMenuTransaction), NULL, this);
	}

	menuFile->AppendSeparator();
	menuFile->Append(static_cast<int>(MainMenuTypes::AddAccount), wxT("New Account..."));
	menuFile->Append(static_cast<int>(MainMenuTypes::AddBudget), wxT("New Budget..."));
	menuFile->Append(static_cast<int>(MainMenuTypes::AddGoal), wxT("New Goal..."));
	menuFile->Append(static_cast<int>(MainMenuTypes::AddScheduler), wxT("New Scheduler..."));
	menuFile->AppendSeparator();
	menuFile->Append(static_cast<int>(MainMenuTypes::Exit), "E&xit\tCtrl+W");

	wxMenuBar *menuBar = new wxMenuBar();

	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");	

	SetMenuBar(menuBar);

	menuHelp->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAbout, this, static_cast<int>(MainMenuTypes::About));
	
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddAccount, this, static_cast<int>(MainMenuTypes::AddAccount));
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddBudget, this, static_cast<int>(MainMenuTypes::AddBudget));
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddGoal, this, static_cast<int>(MainMenuTypes::AddGoal));
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnAddScheduler, this, static_cast<int>(MainMenuTypes::AddScheduler));
	menuFile->Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnQuit, this, static_cast<int>(MainMenuTypes::Exit));
}

void MainFrame::UpdateStatus(wxString text) {
	SetStatusText(text);	
}

void MainFrame::OnQuit(wxCommandEvent &event)
{
	Close(TRUE);
}

void MainFrame::OnAbout(wxCommandEvent &event)
{
	//wxMessageBox(wxString::Format("Version: %s", APP_VERSION), "About Clerk", wxOK | wxICON_INFORMATION, this);
	AboutFrame *aboutFrame = new AboutFrame(this, wxT("About"), 0, 0, 250, 340);

	aboutFrame->Show(true);
	aboutFrame->CenterOnParent();
}

void MainFrame::OnAddAccount(wxCommandEvent &event) {
	AddAccount();
}

void MainFrame::OnTreeMenuAccountSelect(std::shared_ptr<Account> account) {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuAccount, account->id)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuAccount, account->id);
	}
	else {		
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuAccount, account);
	}
}

void MainFrame::OnTreeMenuReportSelect(std::shared_ptr<Report> report) {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuReport, report->id)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuReport, report->id);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuReport, report);
	}
}

void MainFrame::OnTreeMenuDashboardSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuDashboard)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuDashboard);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuDashboard, nullptr);
	}
}

void MainFrame::OnTreeMenuBudgetsSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuBudgets)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuBudgets);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuBudgets, nullptr);
	}
}

void MainFrame::OnTreeMenuGoalsSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuGoals)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuGoals);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuGoals, nullptr);
	}
}

void MainFrame::OnTreeMenuSchedulersSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuSchedulers)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuSchedulers);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuSchedulers, nullptr);
	}
}

void MainFrame::OnTreeMenuTrashSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuTrash)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuTrash);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuTrash, nullptr);
	}
}

void MainFrame::OnTreeMenuTagsSelect() {
	if (tabsPanel->IsTabExists(TreeMenuItemTypes::MenuTags)) {
		tabsPanel->SelectTab(TreeMenuItemTypes::MenuTags);
	}
	else {
		tabsPanel->UpdateCurrentTab(TreeMenuItemTypes::MenuTags, nullptr);
	}
}

void MainFrame::OnTreeMenuAccountsSelect(TreeMenuItemTypes type) {
	if (tabsPanel->IsTabExists(type)) {
		tabsPanel->SelectTab(type);
	}
	else {
		tabsPanel->AddTab(type, nullptr);
	}
}

void MainFrame::OnTreeMenuAddTransaction(std::shared_ptr<Account> account) {
	AddTransaction(account.get());
}

void MainFrame::OnAddTransaction(wxCommandEvent &event) {
	auto account = tabsPanel->GetSelectedAccount();
	AddTransaction(account.get());
}

void MainFrame::OnDuplicateTransaction(wxCommandEvent &event) {
	auto transaction = tabsPanel->GetSelectedTransaction();

	if (transaction) {
		Transaction *copy = new Transaction();

		copy->fromAccountId = transaction->fromAccountId;
		copy->toAccountId = transaction->toAccountId;
		copy->fromAmount = transaction->fromAmount;
		copy->toAmount = transaction->toAmount;
		copy->note = transaction->note;
		copy->tags = transaction->tags;
		copy->paidAt = make_shared<wxDateTime>(wxDateTime::Now());

		copy->Save();

		delete copy;

		tabsPanel->Update();
	}	
}

void MainFrame::OnSplitTransaction(wxCommandEvent &event) {
	auto transaction = tabsPanel->GetSelectedTransaction();

	if (transaction) {
		SplitTransaction(transaction);
	}
}

void MainFrame::AddTransaction(Account *account) {
	auto transaction = make_shared<Transaction>();

	if (account) {
		if (account->type == AccountTypes::Receipt || account->type == AccountTypes::Deposit || account->type == AccountTypes::Virtual) {
			transaction->fromAccountId = account->id;
			transaction->toAccountId = DataHelper::GetInstance().GetPairAccountId(account);
		}

		if (account->type == AccountTypes::Expens || account->type == AccountTypes::Credit) {
			transaction->toAccountId = account->id;
			transaction->fromAccountId = DataHelper::GetInstance().GetPairAccountId(account);
		}
	}

	transactionFrame = new TransactionDialog(this, wxT("Transaction"), 0, 0, 450, 350);

	transactionFrame->SetTransaction(transaction);
	transactionFrame->OnClose = std::bind(&MainFrame::OnTransactionClose, this);

	transactionFrame->Show(true);
	transactionFrame->CenterOnParent();
}

void MainFrame::AddTransactionFromContextMenu() {
	auto transaction = tabsPanel->GetSelectedTransaction();

	if (transaction) {
		auto copy = make_shared<Transaction>();

		copy->fromAccountId = transaction->fromAccountId;
		copy->toAccountId = transaction->toAccountId;		

		EditTransaction(copy);
	}
	else {
		AddTransaction(nullptr);
	}
}

void MainFrame::CopyTransaction(std::shared_ptr<Transaction> transaction) {
	auto copy = make_shared<Transaction>();

	copy->fromAccountId = transaction->fromAccountId;
	copy->toAccountId = transaction->toAccountId;
	copy->fromAmount = transaction->fromAmount;
	copy->toAmount = transaction->toAmount;
	copy->note = transaction->note;
	copy->tags = transaction->tags;
	copy->paidAt = make_shared<wxDateTime>(wxDateTime::Now());

	TransactionDialog *transactionDialog = new TransactionDialog(this, wxT("Transaction"), 0, 0, 450, 350);

	transactionDialog->SetTransaction(copy);
	transactionDialog->OnClose = std::bind(&MainFrame::OnTransactionClose, this);

	transactionDialog->Show(true);
	transactionDialog->CenterOnParent();
}

void MainFrame::EditTransaction(std::shared_ptr<Transaction> transaction) {
	TransactionDialog *transactionDialog = new TransactionDialog(this, wxT("Transaction"), 0, 0, 450, 350);
	
	transactionDialog->SetTransaction(transaction);
	transactionDialog->OnClose = std::bind(&MainFrame::OnTransactionClose, this);

	transactionDialog->Show(true);
	transactionDialog->CenterOnParent();
}

void MainFrame::SplitTransaction(std::shared_ptr<Transaction> transaction) {
	TransactionDialog *transactionDialog = new TransactionDialog(this, wxT("Transaction"), 0, 0, 450, 350);
	
	transactionDialog->SetSplitTransaction(transaction);
	transactionDialog->OnClose = std::bind(&MainFrame::OnTransactionClose, this);

	transactionDialog->Show(true);
	transactionDialog->CenterOnParent();
}

void MainFrame::OnTransactionClose() {
	tabsPanel->Update();
}

void MainFrame::AddAccount() {
	std::shared_ptr<Account> account = make_shared<Account>();
	
	AccountDialog *accountDialog = new AccountDialog(this, wxT("Account"), 0, 0, 340, 400);

	accountDialog->SetAccount(account);
	accountDialog->OnClose = std::bind(&MainFrame::OnAccountClose, this);

	accountDialog->Show(true);
	accountDialog->CenterOnParent();
}

void MainFrame::EditAccount(std::shared_ptr<Account> account) {
	AccountDialog *accountDialog = new AccountDialog(this, wxT("Account"), 0, 0, 340, 400);
		
	accountDialog->SetAccount(account);
	accountDialog->OnClose = std::bind(&MainFrame::OnAccountClose, this);

	accountDialog->Show(true);
	accountDialog->CenterOnParent();
}

void MainFrame::DeleteAccount(std::shared_ptr<Account> account) {
	account->Delete();

	treeMenu->Update();
	treeMenu->RestoreState();
}

void MainFrame::RestoreAccount(std::shared_ptr<Account> account) {
	account->Restore();

	treeMenu->Update();
	treeMenu->RestoreState();
}

void MainFrame::OnAccountClose() {
	treeMenu->Update();
	treeMenu->RestoreState();
}

void MainFrame::OnAddBudget(wxCommandEvent &event) {
	AddBudget();
}

void MainFrame::AddBudget() {
	std::shared_ptr<Budget> budget = make_shared<Budget>();
	
	BudgetDialog *budgetDialog = new BudgetDialog(this, wxT("Budget"), 0, 0, 340, 400);

	budgetDialog->SetBudget(budget);
	budgetDialog->OnClose = std::bind(&MainFrame::OnBudgetClose, this);

	budgetDialog->Show(true);
	budgetDialog->CenterOnParent();
}

void MainFrame::EditBudget(std::shared_ptr<Budget> budget) {
	BudgetDialog *budgetDialog = new BudgetDialog(this, wxT("Budget"), 0, 0, 340, 400);

	budgetDialog->SetBudget(budget);
	budgetDialog->OnClose = std::bind(&MainFrame::OnBudgetClose, this);

	budgetDialog->Show(true);
	budgetDialog->CenterOnParent();
}

void MainFrame::OnBudgetClose() {
	tabsPanel->Update();
}

void MainFrame::OnAddScheduler(wxCommandEvent &event) {
	AddScheduler();
}

void MainFrame::AddScheduler() {
	std::shared_ptr<Scheduler> scheduler = make_shared<Scheduler>();

	SchedulerDialog *schedulerDialog = new SchedulerDialog(this, wxT("Scheduler"), 0, 0, 450, 480);
	
	schedulerDialog->SetScheduler(scheduler);
	schedulerDialog->OnClose = std::bind(&MainFrame::OnSchedulerClose, this);

	schedulerDialog->Show(true);
	schedulerDialog->CenterOnParent();
}

void MainFrame::EditScheduler(std::shared_ptr<Scheduler> scheduler) {
	SchedulerDialog *schedulerDialog = new SchedulerDialog(this, wxT("Scheduler"), 0, 0, 450, 480);

	schedulerDialog->SetScheduler(scheduler);
	schedulerDialog->OnClose = std::bind(&MainFrame::OnSchedulerClose, this);

	schedulerDialog->Show(true);
	schedulerDialog->CenterOnParent();
}

void MainFrame::OnSchedulerClose() {
	tabsPanel->Update();
}

void MainFrame::OnAddGoal(wxCommandEvent &event) {
	AddGoal();
}

void MainFrame::AddGoal() {
	std::shared_ptr<Goal> goal = make_shared<Goal>();

	GoalDialog *goalDialog = new GoalDialog(this, wxT("Goal"), 0, 0, 340, 400);

	goalDialog->SetGoal(goal);
	goalDialog->OnClose = std::bind(&MainFrame::OnGoalClose, this);

	goalDialog->Show(true);
	goalDialog->CenterOnParent();
}

void MainFrame::EditGoal(std::shared_ptr<Goal> goal) {
	GoalDialog *goalDialog = new GoalDialog(this, wxT("Goal"), 0, 0, 340, 400);

	goalDialog->SetGoal(goal);
	goalDialog->OnClose = std::bind(&MainFrame::OnGoalClose, this);

	goalDialog->Show(true);
	goalDialog->CenterOnParent();
}

void MainFrame::OnGoalClose() {
	tabsPanel->Update();
}

void MainFrame::AddTab(TreeMenuItemTypes type, shared_ptr<void> object) {
	tabsPanel->AddTab(type, object);
}

void MainFrame::CheckSchedulers() {
	wxDateTime today = wxDateTime::Now();
	today.ResetTime();

	std::vector<shared_ptr<Scheduler>> schedulers;

	for each (auto scheduler in DataHelper::GetInstance().GetSchedulers())
	{
		if (scheduler->active && (today.IsEqualTo(*scheduler->nextDate) || today.IsLaterThan(*scheduler->nextDate))) {
			schedulers.push_back(scheduler);
		}
	}

	if (schedulers.size() > 0) {
		SchedulersConfirmDialog *confirmDialog = new SchedulersConfirmDialog(this, wxT("Schedulers"), 0, 0, 450, 400);

		confirmDialog->SetSchedulers(schedulers);
		confirmDialog->OnClose = std::bind(&MainFrame::OnSchedulersConfirmClose, this);

		confirmDialog->Show(true);
		confirmDialog->CenterOnParent();
	}
}

void MainFrame::OnSchedulersConfirmClose() {
	tabsPanel->Update();
}

void MainFrame::OnEmptyTrash() {

}

void MainFrame::OnAddMenuTransaction(wxCommandEvent &event) {
	int itemId = event.GetId();

	if (itemId == 0) {
		auto account = tabsPanel->GetSelectedAccount();
		AddTransaction(account.get());
	}
	else {
		auto transaction = make_shared<Transaction>(itemId);
		transaction->fromAmount = 0;
		transaction->toAmount = 0;

		CopyTransaction(transaction);
	}
}