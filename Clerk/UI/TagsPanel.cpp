#include "TagsPanel.h"

TagsPanel::TagsPanel(wxWindow *parent, wxWindowID id) : DataPanel(parent, id) {
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	wxPanel *topPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	wxBoxSizer *boxSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText *searchLabel = new wxStaticText(topPanel, wxID_ANY, wxT("Search:"), wxDefaultPosition, wxDefaultSize, 0);	
	boxSizer->Add(searchLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	searchField = new wxTextCtrl(topPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), 0);
	searchField->Bind(wxEVT_TEXT, &TagsPanel::OnSearchChanged, this);

	boxSizer->Add(searchField, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	topPanel->SetSizer(boxSizer);
	topPanel->Layout();

	boxSizer->Fit(topPanel);
	mainSizer->Add(topPanel, 0, wxEXPAND | wxALL, 5);

	list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_LIST | wxLC_EDIT_LABELS | wxBORDER_NONE);
	list->Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, &TagsPanel::OnListItemBeginEdit, this);
	list->Bind(wxEVT_LIST_END_LABEL_EDIT, &TagsPanel::OnListItemEndEdit, this);
	list->Bind(wxEVT_CONTEXT_MENU, &TagsPanel::OnRightClick, this);
	list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &TagsPanel::OnListItemDoubleClick, this);

	mainSizer->Add(list, 1, wxEXPAND | wxALL, 5);

	this->SetSizer(mainSizer);
	this->Layout();
}

TagsPanel::~TagsPanel() {
	
}

void TagsPanel::Update() {
	std::thread([=]()
	{
		tags = DataHelper::GetInstance().GetTags();
		Filter();

		this->GetEventHandler()->CallAfter(&TagsPanel::UpdateList);
	}).detach();
}

void TagsPanel::Filter() {
	filteredTags.clear();

	if (searchField->GetValue().Length() > 0) {
		wxString search = searchField->GetValue().Lower();		

		for each (auto tag in tags)
		{
			if (tag->name->Find(search) != wxNOT_FOUND) {
				filteredTags.push_back(tag);
			}
		}
	}
	else {
		filteredTags = tags;
	}
}

void TagsPanel::UpdateList() {
	editedIndex = -1;

	list->ClearAll();

	int i = 0;

	for each (auto tag in filteredTags)
	{
		list->InsertItem(i, wxString::Format("%s (%d)", *tag->name, tag->count));
		i++;
	}
}

shared_ptr<Tag> TagsPanel::GetTag() {
	long itemIndex = -1;

	for (;;) {
		itemIndex = list->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		if (itemIndex == -1) {
			break;
		}

		return filteredTags[itemIndex];
	}

	return nullptr;
}

long TagsPanel::GetSelectedIndex() {
	long itemIndex = -1;

	for (;;) {
		itemIndex = list->GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);

		if (itemIndex == -1) {
			break;
		}

		return itemIndex;
	}

	return itemIndex;
}

void TagsPanel::DeleteItemAtIndex(long index) {
	auto tag = filteredTags[index];

	for (int i = 0; i < tags.size(); i++) {
		if (tag->id == tags[i]->id) {
			tags.erase(tags.begin() + i);
			break;
		}
	}

	list->DeleteItem(index);
	filteredTags.erase(filteredTags.begin() + index);
}

void TagsPanel::ShowTransactions() {

}

void TagsPanel::Rename() {
	long index = GetSelectedIndex();

	if (index != -1) {
		editedIndex = index;

		auto tag = filteredTags[index];

		list->SetItemText(index, *tag->name);
		list->EditLabel(index);
	}
}

void TagsPanel::Delete() {
	auto tag = GetTag();

	if (tag) {
		wxMessageDialog *dialog = new wxMessageDialog(this, wxString::Format("You are sure want to delete tag '%s'?", *tag->name), wxT("Delete tag"), wxOK | wxCANCEL | wxCENTER);

		if (dialog->ShowModal() == wxID_OK) {
			tag->Delete();

			long index = GetSelectedIndex();

			if (index != -1) {
				DeleteItemAtIndex(index);
			}
		}
	}
}

void TagsPanel::OnSearchChanged(wxCommandEvent &event) {
	Filter();
	UpdateList();
}

void TagsPanel::OnListItemBeginEdit(wxListEvent &event) {
	if (editedIndex == -1) {
		event.Veto();
	}
}

void TagsPanel::OnListItemEndEdit(wxListEvent &event) {
	if (editedIndex != -1) {
		wxString newValue = event.GetItem().GetText();
		newValue = newValue.Trim(true).Trim(false);

		auto editedTag = filteredTags[editedIndex];

		if (newValue != *editedTag->name) {
			bool isReplaced = false;

			for (int i = 0; i < tags.size(); i++)
			{
				auto tag = tags[i];

				if (newValue == *tag->name) {
					DataHelper::GetInstance().ReplaceTag(editedTag->id, tag->id);
					editedTag->Delete();

					tag->count = tag->count + editedTag->count;
					list->SetItemText(i, wxString::Format("%s (%d)", *tag->name, tag->count));

					isReplaced = true;
				}
			}

			if (isReplaced) {	
				DeleteItemAtIndex(editedIndex);
			} else {
				editedTag->name = std::make_shared<wxString>(newValue);
				editedTag->Save();

				list->SetItemText(editedIndex, wxString::Format("%s (%d)", *editedTag->name, editedTag->count));
			}
		}
		else {
			list->SetItemText(editedIndex, wxString::Format("%s (%d)", *editedTag->name, editedTag->count));
		}

		editedIndex = -1;

		event.Veto();
		list->EndEditLabel(false);
	}
}

void TagsPanel::OnListItemDoubleClick(wxListEvent &event) {
	Rename();
}

void TagsPanel::OnRightClick(wxContextMenuEvent &event) {
	wxMenu *menu = new wxMenu;

	menu->Append(static_cast<int>(TagsPanelMenuTypes::Show), wxT("Show transactions"));
	menu->AppendSeparator();
	menu->Append(static_cast<int>(TagsPanelMenuTypes::Rename), wxT("Rename"));
	menu->Append(static_cast<int>(TagsPanelMenuTypes::Delete), wxT("Delete..."));

	menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &TagsPanel::OnMenuSelect, this);

	wxPoint point = event.GetPosition();
	point = list->ScreenToClient(point);

	list->PopupMenu(menu, point);

	delete menu;
}

void TagsPanel::OnMenuSelect(wxCommandEvent &event) {
	switch (static_cast<TagsPanelMenuTypes>(event.GetId())) {
		case TagsPanelMenuTypes::Show:
			ShowTransactions();
			break;

		case TagsPanelMenuTypes::Rename:
			Rename();
			break;

		case TagsPanelMenuTypes::Delete:
			Delete();
			break;

		default:
			break;
	}
}