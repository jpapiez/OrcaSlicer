#include "MediaPlayCtrl.h"
#include "Widgets/Button.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/Label.hpp"
#include "GUI_App.hpp"
#include "libslic3r/AppConfig.hpp"
#include "I18N.hpp"

namespace Slic3r {
namespace GUI {

MediaPlayCtrl::MediaPlayCtrl(wxWindow *parent, wxMediaCtrl2 *media_ctrl, const wxPoint &pos, const wxSize &size)
    : wxPanel(parent, wxID_ANY, pos, size)
    , m_media_ctrl(media_ctrl)
{
    SetBackgroundColour(*wxWHITE);
    m_media_ctrl->Bind(wxEVT_MEDIA_STATECHANGED, &MediaPlayCtrl::onStateChanged, this);

    m_button_play = new Button(this, "", "media_play", wxBORDER_NONE);
    m_button_play->SetCanFocus(false);

    m_label_status = new Label(this, "", LB_HYPERLINK);

    m_button_play->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [this](auto & e) { TogglePlay(); });

    m_button_play->Bind(wxEVT_RIGHT_UP, [this](auto & e) { m_media_ctrl->Play(); });
    m_label_status->Bind(wxEVT_LEFT_UP, [this](auto &e) {
        auto url = wxString::Format(L"https://wiki.bambulab.com/%s/software/bambu-studio/faq/live-view", L"en");
        wxLaunchDefaultBrowser(url);
    });

    Bind(wxEVT_RIGHT_UP, [this](auto & e) { wxClipboard & c = *wxTheClipboard; if (c.Open()) { c.SetData(new wxTextDataObject(m_url)); c.Close(); } });

    wxBoxSizer * sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_button_play, 0, wxEXPAND | wxALL, 0);
    sizer->AddStretchSpacer(1);
    sizer->Add(m_label_status, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(25));
    SetSizer(sizer);

    m_thread = boost::thread([this] {
        media_proc();
    });

//#if BBL_RELEASE_TO_PUBLIC
//    m_next_retry = wxDateTime::Now();
//#endif

    auto onShowHide = [this](auto &e) {
        e.Skip();
        if (m_isBeingDeleted) return;
        IsShownOnScreen() ? Play() : Stop();
    };
    parent->Bind(wxEVT_SHOW, onShowHide);
    parent->GetParent()->GetParent()->Bind(wxEVT_SHOW, onShowHide);

    m_lan_user = "bblp";
    m_lan_passwd = "bblp";
}

MediaPlayCtrl::~MediaPlayCtrl()
{
    {
        boost::unique_lock lock(m_mutex);
        m_tasks.push_back("<exit>");
        m_cond.notify_all();
    }
    m_thread.join();
}

void MediaPlayCtrl::SetMachineObject(MachineObject* obj)
{
    std::string machine = obj ? obj->dev_id : "";
    if (obj && obj->is_function_supported(PrinterFunction::FUNC_CAMERA_VIDEO)) {
        m_camera_exists = obj->has_ipcam;
        m_lan_mode      = obj->is_lan_mode_printer();
        m_lan_ip       = obj->is_function_supported(PrinterFunction::FUNC_LOCAL_TUNNEL) ? obj->dev_ip : "";
        m_lan_passwd    = obj->is_function_supported(PrinterFunction::FUNC_LOCAL_TUNNEL) ? obj->access_code : "";
        m_tutk_support = obj->is_function_supported(PrinterFunction::FUNC_REMOTE_TUNNEL);
    } else {
        m_camera_exists = false;
        m_lan_mode = false;
        m_lan_ip.clear();
        m_lan_passwd.clear();
        m_tutk_support = true;
    }
    if (machine == m_machine) {
        if (m_last_state == MEDIASTATE_IDLE && m_next_retry.IsValid() && wxDateTime::Now() >= m_next_retry)
            Play();
        return;
    }
    m_machine = machine;
    m_failed_retry = 0;
    if (m_last_state != MEDIASTATE_IDLE)
        Stop();
    if (m_next_retry.IsValid())
        Play();
    else
        SetStatus("", false);
}

void MediaPlayCtrl::Play()
{
    if (!m_next_retry.IsValid())
        return;
    if (!IsShownOnScreen())
        return;
    if (m_last_state != MEDIASTATE_IDLE) {
        return;
    }
    if (m_machine.empty()) {
        Stop();
        SetStatus(_L("Initialize failed (No Device)!"));
        return;
    }
    if (!m_camera_exists) {
        Stop();
        SetStatus(_L("Initialize failed (No Camera Device)!"));
        return;
    }

    m_last_state = MEDIASTATE_INITIALIZING;
    m_button_play->SetIcon("media_stop");
    SetStatus(_L("Initializing..."));

    if (!m_lan_ip.empty()) {
        m_url        = "bambu:///local/" + m_lan_ip + ".?port=6000&user=" + m_lan_user + "&passwd=" + m_lan_passwd;
        m_last_state = MEDIASTATE_LOADING;
        SetStatus(_L("Loading..."));
        if (wxGetApp().app_config->get("dump_video") == "true") {
            std::string file_h264 = data_dir() + "/video.h264";
            std::string file_info = data_dir() + "/video.info";
            BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl dump video to " << file_h264;
            // closed by BambuSource
            FILE *dump_h264_file = boost::nowide::fopen(file_h264.c_str(), "wb");
            FILE *dump_info_file = boost::nowide::fopen(file_info.c_str(), "wb");
            m_url                = m_url + "&dump_h264=" + boost::lexical_cast<std::string>(dump_h264_file);
            m_url                = m_url + "&dump_info=" + boost::lexical_cast<std::string>(dump_info_file);
        }
        boost::unique_lock lock(m_mutex);
        m_tasks.push_back(m_url);
        m_cond.notify_all();
        return;
    }
    
    if (m_lan_mode) {
        Stop();
        SetStatus(m_lan_passwd.empty() 
            ? _L("Initialize failed (Not supported with LAN-only mode)!") 
            : _L("Initialize failed (Not accessible in LAN-only mode)!"));
        return;
    }
    
    if (!m_tutk_support) { // not support tutk
        Stop();
        SetStatus(_L("Initialize failed (Not supported without remote video tunnel)!"));
        return;
    }

    NetworkAgent* agent = wxGetApp().getAgent();
    if (agent) {
        agent->get_camera_url(m_machine, [this, m = m_machine](std::string url) {
            BOOST_LOG_TRIVIAL(info) << "camera_url: " << url;
            CallAfter([this, m, url] {
                if (m != m_machine) return;
                m_url = url;
                if (m_last_state == MEDIASTATE_INITIALIZING) {
                    if (url.empty() || !boost::algorithm::starts_with(url, "bambu:///")) {
                        Stop();
                        SetStatus(wxString::Format(_L("Initialize failed (%s)!"), url.empty() ? _L("Network unreachable") : from_u8(url)));
                    } else {
                        m_last_state = MEDIASTATE_LOADING;
                        SetStatus(_L("Loading..."));
                        if (wxGetApp().app_config->get("dump_video") == "true") {
                            BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl dump video to " << boost::filesystem::current_path();
                            m_url = m_url + "&dump=video.h264";
                        }
                        boost::unique_lock lock(m_mutex);
                        m_tasks.push_back(m_url);
                        m_cond.notify_all();
                    }
                }
            });
        });
    }
}

void MediaPlayCtrl::Stop()
{
    if (m_last_state != MEDIASTATE_IDLE) {
        m_media_ctrl->InvalidateBestSize();
        m_button_play->SetIcon("media_play");
        boost::unique_lock lock(m_mutex);
        m_tasks.push_back("<stop>");
        m_cond.notify_all();
        m_last_state = MEDIASTATE_IDLE;
        if (m_failed_code)
            SetStatus(_L("Stopped [%d]!"), true);
        else
            SetStatus(_L("Stopped."), false);
        if (m_failed_code >= 100) // not keep retry on local error
            m_next_retry = wxDateTime();
    }
    ++m_failed_retry;
    if (m_next_retry.IsValid())
        m_next_retry = wxDateTime::Now() + wxTimeSpan::Seconds(5 * m_failed_retry);
}

void MediaPlayCtrl::TogglePlay()
{
    if (m_last_state != MEDIASTATE_IDLE) {
        m_next_retry = wxDateTime();
        Stop();
    } else {
        m_failed_retry = 0;
        m_next_retry   = wxDateTime::Now();
        Play();
    }
}

void MediaPlayCtrl::SetStatus(wxString const &msg2, bool hyperlink)
{
    auto msg = wxString::Format(msg2, m_failed_code);
    BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl::SetStatus: " << msg.ToUTF8().data();
#ifdef __WXMSW__
    OutputDebugStringA("MediaPlayCtrl::SetStatus: ");
    OutputDebugStringA(msg.ToUTF8().data());
    OutputDebugStringA("\n");
#endif // __WXMSW__
    m_label_status->SetLabel(msg);
    long style = m_label_status->GetWindowStyle() & ~LB_HYPERLINK;
    if (hyperlink) {
        style |= LB_HYPERLINK;
    }
    m_label_status->SetWindowStyle(style);
    m_label_status->InvalidateBestSize();
    Layout();
}

void MediaPlayCtrl::media_proc()
{
    boost::unique_lock lock(m_mutex);
    while (true) {
        while (m_tasks.empty()) {
            m_cond.wait(lock);
        }
        wxString url = m_tasks.front();
        lock.unlock();
        if (url.IsEmpty()) {
            break;
        }
        else if (url == "<stop>") {
            m_media_ctrl->Stop();
        }
        else if (url == "<exit>") {
            break;
        }
        else if (url == "<play>") {
            m_media_ctrl->Play();
        }
        else {
            BOOST_LOG_TRIVIAL(info) <<  "MediaPlayCtrl: start load";
            m_media_ctrl->Load(wxURI(url));
            BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl: end load";
        }
        lock.lock();
        m_tasks.pop_front();
        wxMediaEvent theEvent(wxEVT_MEDIA_STATECHANGED, m_media_ctrl->GetId());
        theEvent.SetId(0);
        m_media_ctrl->GetEventHandler()->AddPendingEvent(theEvent);
    }
}

void MediaPlayCtrl::onStateChanged(wxMediaEvent& event)
{
    auto last_state = m_last_state;
    auto state = m_media_ctrl->GetState();
    BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl::onStateChanged: " << state << ", last_state: " << last_state;
    if ((int) state < 0)
        return;
    {
        boost::unique_lock lock(m_mutex);
        if (!m_tasks.empty()) {
            BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl::onStateChanged: skip when task not finished";
            return;
        }
    }
    if (last_state == MEDIASTATE_IDLE && state == wxMEDIASTATE_STOPPED) {
        return;
    }
    if ((last_state == wxMEDIASTATE_PAUSED || last_state == wxMEDIASTATE_PLAYING)  &&
        state == wxMEDIASTATE_STOPPED) {
        m_failed_code = m_media_ctrl->GetLastError();
        Stop();
        return;
    }
    if (last_state == MEDIASTATE_LOADING && state == wxMEDIASTATE_STOPPED) {
        wxSize size = m_media_ctrl->GetVideoSize();
        BOOST_LOG_TRIVIAL(info) << "MediaPlayCtrl::onStateChanged: size: " << size.x << "x" << size.y;
        m_failed_code = m_media_ctrl->GetLastError();
        if (size.GetWidth() > 1000) {
            m_last_state = state;
            SetStatus(_L("Playing..."), false);
            m_failed_retry = 0;
            boost::unique_lock lock(m_mutex);
            m_tasks.push_back("<play>");
            m_cond.notify_all();
        }
        else if (event.GetId()) {
            Stop();
            if (m_failed_code == 0)
                m_failed_code = 2;
            SetStatus(_L("Load failed [%d]!"));
        }
    } else {
        m_last_state = state;
    }
}

}}

void wxMediaCtrl2::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    if (sizeFlags & wxSIZE_USE_EXISTING) return;
    wxSize size = GetVideoSize();
    if (size.GetWidth() <= 0)
        size = wxSize{16, 9};
    int maxHeight = (width * size.GetHeight() + size.GetHeight() - 1) / size.GetWidth();
    if (maxHeight != GetMaxHeight()) {
        // BOOST_LOG_TRIVIAL(info) << "wxMediaCtrl2::DoSetSize: width: " << width << ", height: " << height << ", maxHeight: " << maxHeight;
        SetMaxSize({-1, maxHeight});
        Slic3r::GUI::wxGetApp().CallAfter([this] {
            if (auto p = GetParent()) {
                p->Layout();
                p->Refresh();
            }
        });
    }
}

