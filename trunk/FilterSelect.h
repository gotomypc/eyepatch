#pragma once

class CFilterSelect : public CDialogImpl<CFilterSelect> {
public:
   enum { IDD = IDD_FILTERSELECT_DIALOG };

    BEGIN_MSG_MAP(CFilterSelect)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_ENABLE, OnEnable)
    END_MSG_MAP()

    CFilterSelect(CWindow *parent);
    ~CFilterSelect();
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEnable(UINT, WPARAM, LPARAM, BOOL&);
private:
    CWindow *parent;
};