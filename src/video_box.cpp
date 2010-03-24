// Copyright (c) 2005, Rodrigo Braz Monteiro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file video_box.cpp
/// @brief The video area in the main window, including surrounding tool bars
/// @ingroup main_ui video
///


////////////
// Includes
#include "config.h"

#ifndef AGI_PRE
#include <wx/filename.h>
#include <wx/mimetype.h>
#include <wx/rawbmp.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "frame_main.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "options.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "toggle_bitmap.h"
#include "utils.h"
#include "vfr.h"
#include "video_box.h"
#include "video_context.h"
#include "video_display.h"
#include "video_provider_manager.h"
#include "video_slider.h"
#include "visual_tool.h"


/// @brief Constructor 
/// @param parent     
/// @param isDetached 
///
VideoBox::VideoBox(wxWindow *parent, bool isDetached) 
: wxPanel (parent,-1)
{
	// Parent
	videoPage = this;

	// Visual controls sub-toolbar
	visualSubToolBar = new wxToolBar(videoPage,-1,wxDefaultPosition,wxDefaultSize,wxTB_HORIZONTAL | wxTB_BOTTOM | wxTB_FLAT);

	// Buttons
	wxBitmapButton *VideoPlayButton = new wxBitmapButton(videoPage,Video_Play,GETIMAGE(button_play_24),wxDefaultPosition,wxSize(25,-1));
	VideoPlayButton->SetToolTip(_("Play video starting on this position"));
	wxBitmapButton *VideoPlayLineButton = new wxBitmapButton(videoPage,Video_Play_Line,GETIMAGE(button_playline_24),wxDefaultPosition,wxSize(25,-1));
	VideoPlayLineButton->SetToolTip(_("Play current line"));
	wxBitmapButton *VideoStopButton = new wxBitmapButton(videoPage,Video_Stop,GETIMAGE(button_pause_24),wxDefaultPosition,wxSize(25,-1));
	VideoStopButton->SetToolTip(_("Stop video playback"));
	AutoScroll = new ToggleBitmap(videoPage,Video_Auto_Scroll,GETIMAGE(toggle_video_autoscroll_24),wxSize(30,-1));
	AutoScroll->SetToolTip(_("Toggle autoscroll of video"));
	AutoScroll->SetValue(OPT_GET("Video/Subtitle Sync")->GetBool());

	// Seek
	videoSlider = new VideoSlider(videoPage,-1);
	videoSlider->SetToolTip(_("Seek video."));

	// Position
	VideoPosition = new wxTextCtrl(videoPage,-1,_T(""),wxDefaultPosition,wxSize(110,20),wxTE_READONLY);
	VideoPosition->SetToolTip(_("Current frame time and number."));

	// Times of sub relative to video
	VideoSubsPos = new wxTextCtrl(videoPage,-1,_T(""),wxDefaultPosition,wxSize(110,20),wxTE_READONLY);
	VideoSubsPos->SetToolTip(_("Time of this frame relative to start and end of current subs."));

	// Typesetting buttons
	visualToolBar = new wxToolBar(videoPage,-1,wxDefaultPosition,wxDefaultSize,wxTB_VERTICAL|wxTB_FLAT|wxTB_NODIVIDER);
	visualToolBar->AddTool(Video_Mode_Standard,_("Standard"),GETIMAGE(visual_standard_24),_("Standard mode, double click sets position."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Drag,_("Drag"),GETIMAGE(visual_move_24),_("Drag subtitles."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Rotate_Z,_("Rotate Z"),GETIMAGE(visual_rotatez_24),_("Rotate subtitles on their Z axis."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Rotate_XY,_("Rotate XY"),GETIMAGE(visual_rotatexy_24),_("Rotate subtitles on their X and Y axes."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Scale,_("Scale"),GETIMAGE(visual_scale_24),_("Scale subtitles on X and Y axes."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Clip,_("Clip"),GETIMAGE(visual_clip_24),_("Clip subtitles to a rectangle."),wxITEM_RADIO);
	visualToolBar->AddTool(Video_Mode_Vector_Clip,_("Vector Clip"),GETIMAGE(visual_vector_clip_24),_("Clip subtitles to a vectorial area."),wxITEM_RADIO);
	visualToolBar->AddSeparator();
	visualToolBar->AddTool(Video_Mode_Realtime,_("Realtime"),GETIMAGE(visual_realtime_24),_("Toggle realtime display of changes."),wxITEM_CHECK);
	visualToolBar->ToggleTool(Video_Mode_Realtime,OPT_GET("Video/Visual Realtime")->GetBool());
	visualToolBar->AddTool(Video_Mode_Help,_("Help"),GETIMAGE(visual_help_24),_("Open the manual page for Visual Typesetting."));
	visualToolBar->Realize();
	// Avoid ugly themed background on Vista and possibly also Win7
	visualToolBar->SetBackgroundStyle(wxBG_STYLE_COLOUR);
	visualToolBar->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

	// Display
	videoDisplay = new VideoDisplay(this,videoSlider,VideoPosition,VideoSubsPos,videoPage,-1,wxDefaultPosition,wxDefaultSize,wxSUNKEN_BORDER);
	VideoContext::Get()->AddDisplay(videoDisplay);
	videoDisplay->Reset();

	// Set display
	videoSlider->Display = videoDisplay;

	// Top sizer
	// Detached and attached video needs different flags, see bugs #742 and #853
	int highSizerFlags = isDetached ? wxEXPAND : 0;
	wxSizer *topTopSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
	visualSubToolBar->Show(false);
	topTopSizer->Add(visualToolBar,0,highSizerFlags,0);
	topTopSizer->Add(videoDisplay,1,highSizerFlags,0);
	topSizer->Add(topTopSizer,1,wxEXPAND,0);
	topSizer->Add(visualSubToolBar,0,wxEXPAND | wxBOTTOM,4);
	topSizer->Add(new wxStaticLine(videoPage),0,wxEXPAND,0);

	// Sizers
	videoSliderSizer = new wxBoxSizer(wxHORIZONTAL);
	videoSliderSizer->Add(videoSlider,1,wxEXPAND|wxLEFT,0);
	wxSizer *videoBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	//videoBottomSizer->Add(zoomSlider,1,wxEXPAND,0);
	videoBottomSizer->Add(VideoPlayButton,0,wxTOP|wxLEFT|wxBOTTOM|wxALIGN_CENTER,2);
	videoBottomSizer->Add(VideoPlayLineButton,0,wxTOP|wxBOTTOM|wxALIGN_CENTER,2);
	videoBottomSizer->Add(VideoStopButton,0,wxTOP|wxBOTTOM|wxALIGN_CENTER,2);
	videoBottomSizer->Add(AutoScroll,0,wxTOP|wxBOTTOM|wxALIGN_CENTER|wxEXPAND,2);
	videoBottomSizer->Add(VideoPosition,1,wxLEFT|wxALIGN_CENTER,5);
	videoBottomSizer->Add(VideoSubsPos,1,wxALIGN_CENTER,0);

	// If we're detached we do want to fill out as much space we can.
	// But if we're in the main window, the subs grid needs space more than us.
	VideoSizer = new wxBoxSizer(wxVERTICAL);
	VideoSizer->Add(topSizer,isDetached?1:0,wxEXPAND,0);
	VideoSizer->Add(videoSliderSizer,0,wxEXPAND,0);
	VideoSizer->Add(videoBottomSizer,0,wxEXPAND,0);
	if (!isDetached)
		VideoSizer->AddStretchSpacer(1);
	SetSizer(VideoSizer);
}

///////////////
// Event table
BEGIN_EVENT_TABLE(VideoBox, wxPanel)
	EVT_BUTTON(Video_Play, VideoBox::OnVideoPlay)
	EVT_BUTTON(Video_Play_Line, VideoBox::OnVideoPlayLine)
	EVT_BUTTON(Video_Stop, VideoBox::OnVideoStop)
	EVT_TOGGLEBUTTON(Video_Auto_Scroll, VideoBox::OnVideoToggleScroll)

	EVT_TOOL_RANGE(Video_Mode_Standard, Video_Mode_Vector_Clip, VideoBox::OnModeChange)
	EVT_TOOL_RANGE(VISUAL_SUB_TOOL_START,VISUAL_SUB_TOOL_END, VideoBox::OnSubTool)
	EVT_TOOL(Video_Mode_Realtime, VideoBox::OnToggleRealtime)
	EVT_TOOL(Video_Mode_Help, VideoBox::OnHelp)
END_EVENT_TABLE()



/// @brief Play video 
/// @param event 
///
void VideoBox::OnVideoPlay(wxCommandEvent &event) {
	VideoContext *ctx = VideoContext::Get();
#ifdef __APPLE__
	ctx->EnableAudioSync(wxGetMouseState().CmdDown() == false);
#else
	ctx->EnableAudioSync(wxGetMouseState().ControlDown() == false);
#endif
	ctx->Play();
}



/// @brief Play video line 
/// @param event 
///
void VideoBox::OnVideoPlayLine(wxCommandEvent &event) {
	VideoContext *ctx = VideoContext::Get();
#ifdef __APPLE__
	ctx->EnableAudioSync(wxGetMouseState().CmdDown() == false);
#else
	ctx->EnableAudioSync(wxGetMouseState().ControlDown() == false);
#endif
	ctx->PlayLine();
}



/// @brief Stop video 
/// @param event 
///
void VideoBox::OnVideoStop(wxCommandEvent &event) {
	VideoContext::Get()->Stop();
}



/// @brief Toggle autoscroll 
/// @param event 
///
void VideoBox::OnVideoToggleScroll(wxCommandEvent &event) {
	Options.SetBool(_T("Sync video with subs"),AutoScroll->GetValue());
	Options.Save();
}



/// @brief Mode changed 
/// @param event 
///
void VideoBox::OnModeChange(wxCommandEvent &event) {
	videoDisplay->SetVisualMode(event.GetId() - Video_Mode_Standard);
}



/// @brief Sub-tool button pressed 
/// @param event 
///
void VideoBox::OnSubTool(wxCommandEvent &event) {
	videoDisplay->OnSubTool(event);
}



/// @brief Realtime toggle 
/// @param event 
///
void VideoBox::OnToggleRealtime(wxCommandEvent &event) {
	Options.SetBool(_T("Video Visual Realtime"),event.IsChecked());
	Options.Save();
}



/// @brief Help 
/// @param event 
///
void VideoBox::OnHelp(wxCommandEvent &event) {
	HelpButton::OpenPage(_T("Visual Typesetting"));
}




