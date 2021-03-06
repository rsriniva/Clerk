#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include "../Data/DataHelper.h"
#include "DataPanel.h"

enum class SchedulersPanelMenuTypes {
	Add = 1,
	Edit = 2,
	Delete = 3,
	Run = 4,
	Pause = 5,
};

class SchedulersPanel : public DataPanel
{
public:
	SchedulersPanel(wxWindow *parent, wxWindowID id);
	
	std::shared_ptr<Scheduler> GetScheduler();
	void Update();	

	std::function<void(std::shared_ptr<Scheduler> scheduler)> OnEdit;
	std::function<void()> OnAdd;

private:
	wxListCtrl *list;
	std::vector<std::shared_ptr<Scheduler>> schedulers;

	void Add();
	void Edit();
	void Delete();
	void Run();
	void Pause();
	void OnListItemDoubleClick(wxListEvent &event);
	void OnRightClick(wxContextMenuEvent &event);
	void OnMenuSelect(wxCommandEvent &event);
};