#include "TransactionsListPanel.h"
#include <algorithm>
#include <string> 

TransactionsListPanel::TransactionsListPanel(wxWindow *parent, wxWindowID id) : wxPanel(parent, id) {
	transactionsList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_NONE);
	transactionsList->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &TransactionsListPanel::OnListItemClick, this);
	transactionsList->Bind(wxEVT_LIST_COL_CLICK, &TransactionsListPanel::OnListColumnClick, this);

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel *filterPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 40));

	wxBoxSizer *filterSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *periodSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText *st3 = new wxStaticText(filterPanel, wxID_ANY, wxT("Period:"));

	wxArrayString *values = new wxArrayString();

	values->Add(wxT("Previous Week"));
	values->Add(wxT("This Week"));
	values->Add(wxT("Previous Month"));
	values->Add(wxT("This Month"));
	values->Add(wxT("This Year"));
	values->Add(wxT("Custom"));

	periodList = new wxComboBox(filterPanel, wxID_ANY, "", wxPoint(0, 0), wxSize(120, 20), *values, wxCB_DROPDOWN | wxCB_READONLY);
	delete values;

	wxStaticText *st1 = new wxStaticText(filterPanel, wxID_ANY, wxT("From:"));
	fromDatePicker = new wxDatePickerCtrl(filterPanel, wxID_ANY, wxDefaultDateTime, wxPoint(0, 0), wxSize(100, 20), wxDP_DROPDOWN);
	wxStaticText *st2 = new wxStaticText(filterPanel, wxID_ANY, wxT("To:"));
	toDatePicker = new wxDatePickerCtrl(filterPanel, wxID_ANY, wxDefaultDateTime, wxPoint(0, 0), wxSize(100, 20), wxDP_DROPDOWN);

	periodSizer->Add(st3, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	periodSizer->Add(periodList, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	periodSizer->Add(st1, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	periodSizer->Add(fromDatePicker, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	periodSizer->Add(st2, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	periodSizer->Add(toDatePicker, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);

	filterSizer->Add(periodSizer, 1, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT | wxEXPAND, 5);

	wxBoxSizer *searchSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText *searchLabel = new wxStaticText(filterPanel, wxID_ANY, wxT("Search:"));
	searchField = new wxTextCtrl(filterPanel, wxID_ANY, "", wxPoint(0, 0), wxSize(200, 20));
	searchField->Bind(wxEVT_TEXT, &TransactionsListPanel::OnSearchChanged, this);

	searchSizer->Add(searchLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	searchSizer->Add(searchField, 0, wxALIGN_CENTER_VERTICAL, 5);

	filterSizer->Add(searchSizer, 0, wxALIGN_RIGHT | wxALL | wxEXPAND, 5);

	filterPanel->SetSizer(filterSizer);
	filterPanel->Layout();

	mainSizer->Add(filterPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 0);
	mainSizer->Add(transactionsList, 1, wxALL | wxEXPAND, 0);

	this->SetSizer(mainSizer);
	this->Layout();

	periodList->Bind(wxEVT_COMBOBOX, &TransactionsListPanel::OnPeriodSelect, this);
	fromDatePicker->Bind(wxEVT_DATE_CHANGED, &TransactionsListPanel::OnDateChanged, this);
	toDatePicker->Bind(wxEVT_DATE_CHANGED, &TransactionsListPanel::OnDateChanged, this);

	periodList->Select(3);
	CalculatePeriod();

	balance = 0.0;
	sortBy = 2;
	sortDesc = false;
}

TransactionsListPanel::~TransactionsListPanel() {
	delete periodList;
	delete fromDatePicker;
	delete toDatePicker;
	delete searchField;
}

void TransactionsListPanel::SetAccount(Account *account) {
	this->account = account;
}

void TransactionsListPanel::SetType(TreeMenuItemTypes type) {
	this->type = type;
}

shared_ptr<Transaction> TransactionsListPanel::GetTransaction() {
	long itemIndex = -1;

	for (;;) {
		itemIndex = transactionsList->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		if (itemIndex == -1) {
			break;
		}

		return transactions[itemIndex];
	}

	return NULL;
}

void TransactionsListPanel::Update() {
	transactionsList->ClearAll();
	transactions.clear();

	if (account == NULL) {
		return;
	}	
		
	wxDateTime date = wxDateTime::Now() - wxTimeSpan::Days(30);
	balance = 0;
	
	if (this->type == TreeMenuItemTypes::MenuAccount) {
		for each (auto transaction in DataHelper::GetInstance().GetTransactions(account, &fromDatePicker->GetValue(), &toDatePicker->GetValue()))
		{
			if (searchField->GetValue().Length() > 0) {
				wxString search = searchField->GetValue().Lower();
				wxString tags = transaction->tags->Lower();
				wxString name;

				if (account->id == transaction->fromAccountId) {
					name = *transaction->toAccountName;
				}

				if (account->id == transaction->toAccountId) {
					name = *transaction->fromAccountName;
				}

				if (tags.Find(search) == wxNOT_FOUND && name.Find(search) == wxNOT_FOUND) {
					continue;
				}
			}

			balance = balance + transaction->toAmount;
			transactions.push_back(transaction);
		}
	} else if (this->type == TreeMenuItemTypes::MenuExpenses) {
		for each (auto transaction in DataHelper::GetInstance().GetTransactionsByType(AccountTypes::Expens, &fromDatePicker->GetValue(), &toDatePicker->GetValue()))
		{
			if (searchField->GetValue().Length() > 0) {
				wxString search = searchField->GetValue().Lower();
				wxString tags = transaction->tags->Lower();
				wxString name = *transaction->toAccountName;

				if (tags.Find(search) == wxNOT_FOUND && name.Find(search) == wxNOT_FOUND) {
					continue;
				}
			}

			balance = balance + transaction->toAmount;
			transactions.push_back(transaction);
		}
	}
	else if (this->type == TreeMenuItemTypes::MenuReceipts) {
		for each (auto transaction in DataHelper::GetInstance().GetTransactionsByType(AccountTypes::Receipt, &fromDatePicker->GetValue(), &toDatePicker->GetValue()))
		{
			if (searchField->GetValue().Length() > 0) {
				wxString search = searchField->GetValue().Lower();
				wxString tags = transaction->tags->Lower();
				wxString name = *transaction->fromAccountName;

				if (tags.Find(search) == wxNOT_FOUND && name.Find(search) == wxNOT_FOUND) {
					continue;
				}
			}

			balance = balance + transaction->toAmount;
			transactions.push_back(transaction);
		}
	}
	else if (this->type == TreeMenuItemTypes::MenuDeposits) {
		for each (auto transaction in DataHelper::GetInstance().GetTransactionsByType(AccountTypes::Deposit, &fromDatePicker->GetValue(), &toDatePicker->GetValue()))
		{
			if (searchField->GetValue().Length() > 0) {
				wxString search = searchField->GetValue().Lower();
				wxString tags = transaction->tags->Lower();
				wxString name = *transaction->fromAccountName;

				if (tags.Find(search) == wxNOT_FOUND && name.Find(search) == wxNOT_FOUND) {
					continue;
				}
			}

			balance = balance + transaction->toAmount;
			transactions.push_back(transaction);
		}
	}

	std::sort(transactions.begin(), transactions.end(), [this](const std::shared_ptr<Transaction>& v1, const std::shared_ptr<Transaction>& v2) {
		if (this->sortBy == 0) {
			return v1->fromAccountName->Cmp(*v2->fromAccountName) == 0;
		}
		else if (this->sortBy == 1) {
			wxString s = v1->tags->Lower();
			wxString s1 = v2->tags->Lower();
			return s.CmpNoCase(s1) == 0;
		}
		else if (this->sortBy == 2) {
			return v1->paidAt->GetMillisecond() < v2->paidAt->GetMillisecond();
		}
		else {
			return v1->fromAmount < v2->fromAmount;
		}
	});

	if (sortDesc) {
		std::reverse(transactions.begin(), transactions.end());
	}

	int index = 0;

	wxListItem column;

	column.SetId(index);
	column.SetText(_("Account"));
	column.SetWidth(200);

	transactionsList->InsertColumn(index, column);

	if (this->type != TreeMenuItemTypes::MenuAccount) {
		column.SetText(_("From Account"));

		wxListItem column1;

		column1.SetId(index++);
		column1.SetText(_("To Account"));
		column1.SetWidth(200);

		transactionsList->InsertColumn(index, column1);
	}
	
	wxListItem column2;

	column2.SetId(index++);
	column2.SetText(_("Tags"));
	column2.SetWidth(200);

	transactionsList->InsertColumn(index, column2);

	wxListItem column3;

	column3.SetId(index++);
	column3.SetText(_("Date"));
	column3.SetWidth(100);
	column3.SetStateMask(wxLIST_MASK_STATE);
	column3.SetState(wxLIST_STATE_SELECTED);

	transactionsList->InsertColumn(index, column3);

	wxListItem column4;

	column4.SetId(index++);
	column4.SetText(_("Amount"));
	column4.SetWidth(100);
	column4.SetAlign(wxLIST_FORMAT_RIGHT);

	transactionsList->InsertColumn(index, column4);

	int i = 0;

	for each (auto transaction in transactions)
	{
		wxListItem listItem;

		listItem.SetId(i);
		listItem.SetData(transaction->id);

		transactionsList->InsertItem(listItem);

		int column = 0;

		if (this->type == TreeMenuItemTypes::MenuAccount) {
			if (account->id == transaction->fromAccountId) {
				transactionsList->SetItem(i, column++, *transaction->toAccountName);
			}

			if (account->id == transaction->toAccountId) {
				transactionsList->SetItem(i, column++, *transaction->fromAccountName);
			}
		} else {
			transactionsList->SetItem(i, column++, *transaction->fromAccountName);
		}		

		if (this->type != TreeMenuItemTypes::MenuAccount) {			
			transactionsList->SetItem(i, column++, *transaction->toAccountName);
		}		
		
		transactionsList->SetItemImage(i, 1);
		transactionsList->SetItem(i, column++, *transaction->tags);
		transactionsList->SetItem(i, column++, transaction->paidAt->Format("%B %d"));

		if (this->type == TreeMenuItemTypes::MenuAccount) {
			if (account->id == transaction->fromAccountId) {
				transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->fromAmount));
			}
			else if (account->id == transaction->toAccountId) {
				transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->toAmount));
			}
			else {
				transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->fromAmount));
			}
		}
		else if (this->type == TreeMenuItemTypes::MenuExpenses) {
			transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->toAmount));
		}
		else if (this->type == TreeMenuItemTypes::MenuReceipts) {
			transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->fromAmount));
		}
		else if (this->type == TreeMenuItemTypes::MenuDeposits) {
			transactionsList->SetItem(i, column++, wxString::Format("%.2f", transaction->fromAmount));
		}

		if (i % 2 == 0) {
			transactionsList->SetItemBackgroundColour(i, wxColor(255, 255, 255, 1));
		}
		else {
			transactionsList->SetItemBackgroundColour(i, wxColor(225, 235, 250, 1));
		}

		i++;
	}
}

void TransactionsListPanel::EditTransaction() {
	if (OnEditTransaction) {
		OnEditTransaction();
	}
}

void TransactionsListPanel::DeleteTransaction() {
	auto transaction = GetTransaction();

	if (transaction) {
		transaction->Delete();
		Update();
	}
}

void TransactionsListPanel::DublicateTransaction() {
	auto transaction = GetTransaction();

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

		Update();
	}
}

void TransactionsListPanel::SplitTransaction() {
	if (OnSplitTransaction) {
		OnSplitTransaction();
	}
}

void TransactionsListPanel::OnListItemClick(wxListEvent &event) {
	wxMenu *menu = new wxMenu;

	menu->Append(ID_EditTransaction, wxT("Edit..."));
	menu->Append(ID_DublicateTransaction, wxT("Dublicate"));
	menu->Append(ID_SplitTransaction, wxT("Split..."));
	menu->AppendSeparator();
	menu->Append(ID_DeleteTransaction, wxT("Delete..."));

	void *data = reinterpret_cast<void *>(event.GetItem().GetData());
	menu->SetClientData(data);

	menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &TransactionsListPanel::OnMenuSelect, this);

	transactionsList->PopupMenu(menu, event.GetPoint());

	delete menu;
}

void TransactionsListPanel::OnListColumnClick(wxListEvent &event) {
	if (sortBy == event.GetColumn()) {
		sortDesc = !sortDesc;
	}

	sortBy = event.GetColumn();
	Update();
}

void TransactionsListPanel::OnMenuSelect(wxCommandEvent &event) {
	switch (event.GetId()) {
		case ID_AddTransaction:
			//AddTransaction();
			break;

		case ID_EditTransaction:
			EditTransaction();
			break;

		case ID_DeleteTransaction:
			DeleteTransaction();
			break;

		case ID_DublicateTransaction:
			DublicateTransaction();
			break;

		case ID_SplitTransaction:
			SplitTransaction();
			break;

		default:
			break;
	}
}

void TransactionsListPanel::OnPeriodSelect(wxCommandEvent &event) {
	CalculatePeriod();
	
	if (OnPeriodChanged) {
		OnPeriodChanged();
	}
}

void TransactionsListPanel::OnDateChanged(wxDateEvent &event) {
	Settings::GetInstance().SetFromPeriodDate(GetFromDate());
	Settings::GetInstance().SetToPeriodDate(GetToDate());

	if (OnPeriodChanged) {
		OnPeriodChanged();
	}
}

void TransactionsListPanel::OnSearchChanged(wxCommandEvent &event) {
	if (OnPeriodChanged) {
		OnPeriodChanged();
	}
}

wxDateTime TransactionsListPanel::GetFromDate() {
	return fromDatePicker->GetValue();
}

wxDateTime TransactionsListPanel::GetToDate() {
	return toDatePicker->GetValue();
}

void TransactionsListPanel::CalculatePeriod() {
	int index = periodList->GetSelection();

	wxDateTime fromDate = wxDateTime::Now();
	wxDateTime toDate = wxDateTime::Now();

	fromDatePicker->Disable();
	toDatePicker->Disable();

	switch (index)
	{
	case 0:
		fromDate.SetToPrevWeekDay(wxDateTime::WeekDay::Sun).SetToWeekDayInSameWeek(wxDateTime::WeekDay::Mon);
		toDate.SetToPrevWeekDay(wxDateTime::WeekDay::Sun);
		break;

	case 1:
		fromDate.SetToWeekDayInSameWeek(wxDateTime::WeekDay::Mon);
		toDate.SetToWeekDayInSameWeek(wxDateTime::WeekDay::Sun);
		break;

	case 2:
		fromDate.Subtract(wxDateSpan::wxDateSpan(0, 1, 0, 0)).SetDay(1);
		toDate.Subtract(wxDateSpan::wxDateSpan(0, 1, 0, 0)).SetToLastMonthDay();
		break;

	case 3:
		fromDate.SetDay(1);
		toDate.SetToLastMonthDay();
		break;

	case 4:
		fromDate.SetMonth(wxDateTime::Month::Jan);
		fromDate.SetDay(1);
		toDate.SetMonth(wxDateTime::Month::Dec);
		toDate.SetDay(31);
		break;

	case 5:
		fromDate = Settings::GetInstance().GetFromPeriodDate();
		toDate = Settings::GetInstance().GetToPeriodDate();

		fromDatePicker->Enable();
		toDatePicker->Enable();
		break;

	default:
		break;
	}

	fromDatePicker->SetValue(fromDate);
	toDatePicker->SetValue(toDate);
}

float TransactionsListPanel::GetBalance() {
	return balance;
}