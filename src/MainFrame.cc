
#include "MainFrame.hh"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <signal.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/process.h>
#include <wx/stdpaths.h>
#include <wx/stream.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>
#include <wx/webview.h>

#include "callback.hh"
#include "config.h"
#include "util.hh"
#include "MainApp.hh"


/**
 * Utility section
 */

/// Load an image file and process it into an appropriate toolbar icon
static wxBitmap toolbar_icon(std::string filename)
{
    return wxBitmap(wxImage(filename).Rescale(config::toolbar_width, config::toolbar_height, wxIMAGE_QUALITY_BICUBIC));
}

/// Click a menu item by name (hidden in our webview)
static void jupyter_click_cell(wxWebView *wv, std::string id)
{
    std::string cmd = "Jupyter.menubar.element.find('#" + id + "').click();";
    wv->RunScript(cmd);
}

/**
 * MainFrame functions
 */

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_WEBVIEW_ERROR(wxID_ANY, MainFrame::OnError)
    EVT_WEBVIEW_TITLE_CHANGED(wxID_ANY, MainFrame::OnTitleChanged)
    EVT_WEBVIEW_NEWWINDOW(wxID_ANY, MainFrame::OnNewWindow)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(std::string url0, std::string filename,
    const wxString &title, const wxPoint &pos, const wxSize &size,
    bool indirect_load)
        : wxFrame(nullptr, wxID_ANY, title, pos, size),
          url(url0), local_filename(filename)
{
    menubar = new wxMenuBar();
    wxMenu *menu_file = new wxMenu();
    menu_file->Append(wxID_NEW, "&New");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_OPEN, "&Open...");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_SAVE, "&Save");
    menu_file->Append(wxID_SAVE_AS, "Save As...");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_SAVE_HTML, "Download HTML");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_PROPERTIES, "Properties...");
    menu_file->AppendSeparator();
    menu_file->Append(wxID_EXIT, "&Quit");
    menubar->Append(menu_file, "&File");

    wxMenu *menu_edit = new wxMenu();
    menu_edit->Append(wxID_CUT, "Cut cell");
    menu_edit->Append(wxID_COPY, "Copy cell");
    menu_edit->Append(wxID_PASTE, "Paste cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_INSERT, "Insert cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_DELETE, "Delete cell");
    menu_edit->Append(wxID_UNDELETE, "Undelete cell");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_SPLIT, "Split cell");
    menu_edit->Append(wxID_MERGE, "Merge cell below");
    menu_edit->AppendSeparator();
    menu_edit->Append(wxID_MOVE_UP, "Move cell up");
    menu_edit->Append(wxID_MOVE_DOWN, "Move cell down");
    menubar->Append(menu_edit, "&Edit");

    wxMenu *menu_cell = new wxMenu();
    menu_cell->Append(wxID_RUN, "Run");
    menu_cell->AppendSeparator();
    menu_cell->Append(wxID_RUN_ALL, "Run all");
    menu_cell->Append(wxID_RUN_ALL_ABOVE, "Run all above");
    menu_cell->Append(wxID_RUN_ALL_BELOW, "Run all below");

    wxMenu *menu_type = new wxMenu();
    menu_type->Append(wxID_CELL_CODE, "Code");
    menu_type->Append(wxID_CELL_MARKDOWN, "Markdown");
    menu_type->Append(wxID_CELL_RAW, "Raw");
    menu_cell->AppendSubMenu(menu_type, "Cell type");

    menubar->Append(menu_cell, "&Cell");

    wxMenu *menu_kernel = new wxMenu();
    menu_kernel->Append(wxID_KERNEL_INTERRUPT, "Interrupt");
    menu_kernel->Append(wxID_KERNEL_RESTART, "Restart");
    menu_kernel->Append(wxID_KERNEL_RECONNECT, "Reconnect");
    menubar->Append(menu_kernel, "&Kernel");

    wxMenu *menu_help = new wxMenu();
    menu_help->Append(wxID_HELP_KEYBOARD, "Keyboard shortcuts");
    menu_help->AppendSeparator();
    menu_help->Append(wxID_HELP_NOTEBOOK, "Notebook");
    menu_help->Append(wxID_HELP_MARKDOWN, "Markdown");
    menu_help->AppendSeparator();
    menu_help->Append(wxID_ABOUT, "&About");
    menu_help->AppendSeparator();

    toolbar = CreateToolBar(config::toolbar_style);

    toolbar->AddTool(wxID_SAVE, "Save", toolbar_icon("images/Save-50.png"), "Save");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_INSERT, "Insert", toolbar_icon("images/Plus-50.png"), "Insert below");
    toolbar->AddTool(wxID_DELETE, "Delete", toolbar_icon("images/Delete-50.png"), "Delete cell");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_CUT, "Cut", toolbar_icon("images/Cut-50.png"), "Cut cell");
    toolbar->AddTool(wxID_COPY, "Copy", toolbar_icon("images/Copy-50.png"), "Copy cell");
    toolbar->AddTool(wxID_PASTE, "Paste", toolbar_icon("images/Paste-50.png"), "Paste cell");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_MOVE_UP, "Move up", toolbar_icon("images/Up-50.png"), "Move cell up");
    toolbar->AddTool(wxID_MOVE_DOWN, "Move down", toolbar_icon("images/Down-50.png"), "Move cell down");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_RUN_NEXT, "Run", toolbar_icon("images/Play-50.png"), "Run cell");
    toolbar->AddTool(wxID_RUN_ALL, "Run all", toolbar_icon("images/FastForward-50.png"), "Run all cells");
    toolbar->AddTool(wxID_KERNEL_INTERRUPT, "Stop", toolbar_icon("images/Stop-50.png"), "Interrupt kernel");
    toolbar->AddTool(wxID_KERNEL_RESTART, "Restart", toolbar_icon("images/Synchronize-50.png"), "Restart kernel");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_CELL_CODE, "Code", toolbar_icon("images/Edit-50.png"), "Cell type code");
    toolbar->AddTool(wxID_CELL_MARKDOWN, "Markdown", toolbar_icon("images/Pen-50.png"), "Cell type markdown");
    toolbar->AddTool(wxID_CELL_RAW, "Raw", toolbar_icon("images/Fantasy-50.png"), "Cell type raw");

    toolbar->AddSeparator();

    toolbar->AddTool(wxID_KERNEL_BUSY, "Busy",
        toolbar_icon("images/Led-Yellow-On-32.png"),
        toolbar_icon("images/Led-Yellow-Off-32.png"),
        wxITEM_NORMAL, "Kernel busy");
    toolbar->EnableTool(wxID_KERNEL_BUSY, false);

    toolbar->Realize();

/*
        {
            'text': "Python",
            'url': "http://docs.python.org/%i.%i" % sys.version_info[:2],
        },
        {
            'text': "IPython",
            'url': "http://ipython.org/documentation.html",
        },
        {
            'text': "NumPy",
            'url': "http://docs.scipy.org/doc/numpy/reference/",
        },
        {
            'text': "SciPy",
            'url': "http://docs.scipy.org/doc/scipy/reference/",
        },
        {
            'text': "Matplotlib",
            'url': "http://matplotlib.org/contents.html",
        },
        {
            'text': "SymPy",
            'url': "http://docs.sympy.org/latest/index.html",
        },
        {
            'text': "pandas",
            'url': "http://pandas.pydata.org/pandas-docs/stable/",
        },
*/

    handler.register_callback(config::token_kernel_busy, AsyncResult::Success,
        [this](Callback::argument x) -> bool {
            std::cout << "KERNEL BUSY " << x << std::endl;
            if (x == std::string("true")) {
                this->toolbar->EnableTool(wxID_KERNEL_BUSY, true);
            }
            if (x == std::string("false")) {
                this->toolbar->EnableTool(wxID_KERNEL_BUSY, false);
            }
            return false; // keep handler alive permanently
        });

    menubar->Append(menu_help, "&Help");
    
    SetMenuBar(menubar);

    Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnMenuEvent, this);

    wxBoxSizer* frame_sizer = new wxBoxSizer(wxVERTICAL);

    webview = wxWebView::New(this, wxID_ANY);
    webview->EnableContextMenu(false);
    
    frame_sizer->Add(webview, 1, wxEXPAND, 10);

    if (indirect_load) {
        if (wxGetApp().load_page.size() == 0) {
            // Read loading page
            std::ifstream ifs(config::loading_html_filename);
            wxGetApp().load_page = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        }
        // Do template replacement for url
        std::string contents{wxGetApp().load_page};
        replace_one(contents, "{{url}}", url);
        webview->SetPage(wxString(contents), "");
    } else {
        webview->LoadURL(url);
    }

    webview->Show();

    SetSizerAndFit(frame_sizer);
    SetSize(wxDefaultCoord, wxDefaultCoord, size.GetWidth(), size.GetHeight());
}

void MainFrame::OnError(wxWebViewEvent &event)
{
    std::cout << "ERROR" << std::endl;
    webview->LoadURL(url);
}

void MainFrame::eval_javascript(std::string expression, Callback::t success, Callback::t failure)
{
    static CallbackHandler::token id = 0;
    id++;
    handler.register_callback(id, AsyncResult::Success, success);
    handler.register_callback(id, AsyncResult::Failure, failure);
    std::stringstream ss;
    ss << "document.title=\"" << config::protocol_prefix << id << "|\"+JSON.stringify(" << expression << ");";
    std::cout << "Trying to eval " << ss.str() << std::endl;
    webview->RunScript(ss.str());
}

void MainFrame::OnMenuEvent(wxCommandEvent &event)
{
    std::cout << "MENU EVENT" << std::endl;
    switch (event.GetId()) {
        case wxID_CUT:
        {
            jupyter_click_cell(webview, "cut_cell");
            break;
        }
        case wxID_COPY:
        {
            jupyter_click_cell(webview, "copy_cell");
            break;
        }
        case wxID_PASTE:
        {
            jupyter_click_cell(webview, "paste_cell_below");
            break;
        }
        case wxID_INSERT:
        {
            jupyter_click_cell(webview, "insert_cell_below");
            break;
        }
        case wxID_DELETE:
        {
            jupyter_click_cell(webview, "delete_cell");
            break;
        }
        case wxID_UNDELETE:
        {
            jupyter_click_cell(webview, "undelete_cell");
            break;
        }
        case wxID_SPLIT:
        {
            jupyter_click_cell(webview, "split_cell");
            break;
        }
        case wxID_MERGE:
        {
            jupyter_click_cell(webview, "merge_cell_below");
            break;
        }
        case wxID_MOVE_UP:
        {
            jupyter_click_cell(webview, "move_cell_up");
            break;
        }
        case wxID_MOVE_DOWN:
        {
            jupyter_click_cell(webview, "move_cell_down");
            break;
        }
        case wxID_RUN:
        {
            jupyter_click_cell(webview, "run_cell");
            break;
        }
        case wxID_RUN_NEXT:
        {
            jupyter_click_cell(webview, "run_cell_select_below");
            break;
        }
        case wxID_RUN_ALL:
        {
            jupyter_click_cell(webview, "run_all_cells");
            break;
        }
        case wxID_RUN_ALL_ABOVE:
        {
            jupyter_click_cell(webview, "run_all_cells_above");
            break;
        }
        case wxID_RUN_ALL_BELOW:
        {
            jupyter_click_cell(webview, "run_all_cells_below");
            break;
        }
        case wxID_CELL_CODE:
        {
            jupyter_click_cell(webview, "to_code");
            break;
        }
        case wxID_CELL_MARKDOWN:
        {
            jupyter_click_cell(webview, "to_markdown");
            break;
        }
        case wxID_CELL_RAW:
        {
            jupyter_click_cell(webview, "to_raw");
            break;
        }
        case wxID_KERNEL_INTERRUPT:
        {
            jupyter_click_cell(webview, "int_kernel");
            break;
        }
        case wxID_KERNEL_RESTART:
        {
            jupyter_click_cell(webview, "restart_kernel");
            break;
        }
        case wxID_KERNEL_RECONNECT:
        {
            jupyter_click_cell(webview, "reconnect_kernel");
            break;
        }
        case wxID_NEW:
        {
            CreateNew();
            break;
        }
        case wxID_OPEN:
        {
            std::cout << "OPEN" << std::endl;
            OnOpen();
            break;
        }
        case wxID_SAVE:
        {
            jupyter_click_cell(webview, "save_checkpoint");
            break;
        }
        case wxID_SAVE_AS:
        {
            std::cout << "SAVE AS" << std::endl;
            OnSaveAs();
            break;
        }
        case wxID_SAVE_HTML:
        {
            jupyter_click_cell(webview, "download_markdown");
            break;
        }
        case wxID_HELP_KEYBOARD:
        {
            jupyter_click_cell(webview, "keyboard_shortcuts");
            break;
        }
        case wxID_HELP_NOTEBOOK:
        {
            wxLaunchDefaultBrowser("http://nbviewer.ipython.org/github/ipython/ipython/blob/3.x/examples/Notebook/Index.ipynb");
            break;
        }
        case wxID_HELP_MARKDOWN:
        {
            wxLaunchDefaultBrowser("https://help.github.com/articles/markdown-basics/");
            break;
        }
        case wxID_ABOUT:
        {
            std::stringstream ss;
            ss << config::version_full << "\n\nCopyright (c) 2015 Nathan Whitehead\n\n" << wxGetLibraryVersionInfo().ToString() << std::endl;
            ss << "Icons are from: https://icons8.com/" << std::endl; 
            wxMessageBox(ss.str(), "About", wxOK | wxICON_INFORMATION);
            break;
        }
        case wxID_EXIT:
        {
            Close(true);
            break;
        }
        case wxID_PROPERTIES:
        {
            eval_javascript(std::string("2+2"), [](Callback::argument x) -> bool {
                std::cout << "Result of 2+2 is " << x << std::endl;
                return true;
            });

            std::stringstream ss;
            ss << "Name: " << std::endl;
            wxMessageBox(ss.str(), "Properties", wxOK | wxICON_INFORMATION);
            break;            
        }
        default:
        {
            std::cerr << "ERROR UNHANDLED MENU EVENT" << std::endl;
            break;
        }
    }
}

void MainFrame::OnClose(wxCloseEvent &event)
{
    std::cout << "CLOSE" << std::endl;
    std::cout << "CLOSING WEBVIEW" << std::endl;
    if (webview) {
        webview->Destroy();
    }
    std::cout << "DESTROY SELF" << std::endl;
    Destroy();
}

void MainFrame::OnTitleChanged(wxWebViewEvent &event)
{
    std::string title = event.GetString().ToStdString();
    std::cout << "TITLE CHANGED - " << title << std::endl;
    // Check if starts with special prefix
    std::string prefix = config::protocol_prefix;
    if (std::equal(prefix.begin(), prefix.end(), title.begin())) {
        // Prefix present
        std::string txt = title.substr(prefix.size());
        std::vector<std::string> items(split(txt, '|'));
        if (items.size() < 2) {
            std::cerr << "SPECIAL TITLE MALFORMED - " << txt << std::endl;
            return;
        }
        CallbackHandler::token id;
        try {
            id = std::stoi(items[0]);
        } catch (...) {
            std::cerr << "SPECIAL TITLE MALFORMED - " << txt << std::endl;
            return;
        }
        handler.call(id, AsyncResult::Success, items[1]);
        return;
    }
    // Otherwise actually change the title
    SetLabel(config::title_prefix + title);
}

MainFrame *MainFrame::Spawn(std::string url, std::string filename, bool indirect_load)
{
    MainFrame *child = new MainFrame(url, filename, url,
        wxPoint(wxDefaultCoord, wxDefaultCoord),
        wxSize(config::initial_width, config::initial_height), indirect_load);
    child->Show();
    return child;
}

void MainFrame::OnNewWindow(wxWebViewEvent &event)
{
    wxString url(event.GetURL());
    std::cout << "NEW WINDOW " << url << std::endl;
    wxLaunchDefaultBrowser(url);
}

MainFrame *MainFrame::CreateNew(bool indirect_load)
{
    std::cout << "CREATE NEW" << std::endl;
    wxString datadir = wxStandardPaths::Get().GetAppDocumentsDir();
    int n = 1;
    wxFileName fullname;
    do {
        std::stringstream ss;
        ss << config::untitled_prefix << n << config::untitled_suffix;
        fullname = wxFileName(datadir, ss.str());
        std::cout << "TRYING " << fullname.GetFullPath() << std::endl;
        if (!fullname.IsOk()) break;
        if (fullname.IsOk() && !fullname.FileExists()) break;
        if (n > config::max_num_untitled) break;
        n++;
    } while (1);
    if (fullname.IsOk() && !fullname.FileExists()) {
        std::cout << "FILENAME " << fullname.GetFullPath() << std::endl;
        // FIXME: drive must be the same as we mounted for windows!!!
        std::ofstream out(fullname.GetFullPath());
        out << wxGetApp().blank_notebook << std::endl;
        // Get path in UNIX so it is a URI
        std::string uri(fullname.GetFullPath(wxPATH_UNIX));
        // Open new window for it
        return Spawn(url_from_filename(uri), std::string(fullname.GetFullPath()), indirect_load);
    }
    std::stringstream ss;
    ss << "Could not create new untitled notebook in ";
    ss << wxStandardPaths::Get().GetAppDocumentsDir() << "\n\n";
    ss << "Last attempt was to create " << std::string(fullname.GetFullPath()) << std::endl;
    wxMessageBox(ss.str(), "ERROR", wxOK | wxICON_ERROR);

    return nullptr;
}

void MainFrame::OnOpen()
{
    wxFileDialog dialog(this, "Open Notebook file", "", "",
        "Notebook files (*.ipynb)|*.ipynb", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_CANCEL) return;

    std::string filename = std::string(dialog.GetPath());
    std::cout << "OPEN " << filename << std::endl;
    Spawn(url_from_filename(filename), filename, false);
}

void MainFrame::OnSaveAs()
{
    wxFileDialog dialog(this, "Save Notebook file", "", "",
        "Notebook files (*.ipynb)|*.ipynb", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() == wxID_CANCEL) return;

    std::string new_filename = std::string(dialog.GetPath());
    std::cout << "SAVE AS " << local_filename << " -> " << new_filename << std::endl;

    std::ifstream ifs(local_filename);
    std::string contents = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    std::ofstream ofs(new_filename);
    ofs << contents;
    local_filename = new_filename;
    url = url_from_filename(local_filename);
    webview->LoadURL(url);
}