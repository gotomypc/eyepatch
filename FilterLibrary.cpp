#include "precomp.h"
#include "constants.h"
#include "TrainingSample.h"
#include "TrainingSet.h"
#include "Classifier.h"
#include "FilterLibrary.h"

CFilterLibrary::CFilterLibrary(CWindow *caller) {
    parent = caller;
}

CFilterLibrary::~CFilterLibrary(void) {
}

LRESULT CFilterLibrary::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return 0;
}

LRESULT CFilterLibrary::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    parent->SendMessage(uMsg, wParam, lParam);
    return 0;
}

LRESULT CFilterLibrary::OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    GetDlgItem(IDC_RUNLIVE).EnableWindow(wParam);
    GetDlgItem(IDC_RUNRECORDED).EnableWindow(wParam);
    GetDlgItem(IDC_RESET).EnableWindow(wParam);
    return 1;
}

LRESULT CFilterLibrary::OnTextCallback(int idCtrl, LPNMHDR pnmh, BOOL&) {
    NMLVDISPINFO* plvdi = (NMLVDISPINFO*)pnmh;
    LVITEM item = plvdi->item;
    switch (item.iSubItem) {
        case 0:
            plvdi->item.pszText = ((Classifier*)item.lParam)->GetName();
            break;
        default:
            break;
    }

    return 0;
}

LRESULT CFilterLibrary::OnNameChange(int idCtrl, LPNMHDR pnmh, BOOL&) {
    NMLVDISPINFO* plvdi = (NMLVDISPINFO*)pnmh;
    LVITEM item = plvdi->item;
	if (item.pszText != NULL) {
		switch (item.iSubItem) {
			case 0:
				((Classifier*)item.lParam)->SetName(item.pszText);
				break;
			default:
				break;
		}
		((Classifier*)item.lParam)->Save();
	}
    return 0;
}

LRESULT CFilterLibrary::OnItemActivate(int idCtrl, LPNMHDR pnmh, BOOL&) {
    
    LPNMLISTVIEW nmlv = (LPNMLISTVIEW) pnmh;
    if (idCtrl == IDC_MY_FILTER_LIST) {

        // we activated an item from the list of custom filters
        HWND listView = GetDlgItem(IDC_MY_FILTER_LIST);

        LV_ITEM lvi;
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        lvi.iItem = nmlv->iItem;
        lvi.iSubItem = 0;
        if (ListView_GetItem(listView, &lvi)) {

            // filter was double-clicked, so we will add it to the list of filters
            Classifier *classifier = (Classifier*) lvi.lParam;
            parent->SendMessage(WM_COMMAND, ((WPARAM)classifier->classifierType), ((LPARAM)classifier));
        }
    }
    return 0;
}

LRESULT CFilterLibrary::OnItemKeyDown(int idCtrl, LPNMHDR pnmh, BOOL&) {

    LPNMLVKEYDOWN nmlv = (LPNMLVKEYDOWN) pnmh;
    HWND listView = GetDlgItem(idCtrl);

    if (idCtrl == IDC_ACTIVE_FILTER_LIST) {

        // keypress was on an item in the list of active filters
        if (nmlv->wVKey == VK_DELETE) {

            // TODO: define a custom message to notify the parent of removal of
            // filter from active list (so it can be removed from VideoRunner)
        }
    }
    return 0;
}

void CFilterLibrary::AddCustomFilter(HWND listView, Classifier* classifier) {

    LVITEM lvi;
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM) classifier;
    lvi.pszText = LPSTR_TEXTCALLBACK;
    ListView_InsertItem(listView, &lvi);
}

void CFilterLibrary::RemoveCustomFilter(HWND listView, Classifier* classifier) {
    int numItems = ListView_GetItemCount(listView);
    int iSelection = -1;
    for (int iIndex=0; iIndex<numItems; iIndex++) {

        // find index of next item
        iSelection = ListView_GetNextItem(listView, iSelection, LVNI_ALL);

        // delete the selected item if it matches the classifier passed in
        LV_ITEM lvi;
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        lvi.iItem = iSelection;
        lvi.iSubItem = 0;
        if (ListView_GetItem(listView, &lvi)) {
            Classifier *current = (Classifier*) lvi.lParam;
            if (current == classifier) {
                ListView_DeleteItem(listView, iSelection);
                break;
            }
        }
    }
}

void CFilterLibrary::ClearCustomFilters(HWND listView) {
    ListView_DeleteAllItems(listView);
}
