/////////////////////////////////////////////////////////////////////////////
// Name:        frame.cpp
// Purpose:
// Author:      Robert Roebling
// Id:          $Id$
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
    #pragma implementation "frame.h"
#endif

#include "wx/defs.h"

#include "wx/dialog.h"
#include "wx/control.h"
#include "wx/app.h"
#include "wx/menu.h"
#if wxUSE_TOOLBAR
    #include "wx/toolbar.h"
#endif
#if wxUSE_STATUSBAR
    #include "wx/statusbr.h"
#endif
#include "wx/dcclient.h"

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

#include "wx/gtk/win_gtk.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

const int wxMENU_HEIGHT    = 27;
const int wxSTATUS_HEIGHT  = 25;
const int wxPLACE_HOLDER   = 0;

// ----------------------------------------------------------------------------
// idle system
// ----------------------------------------------------------------------------

extern void wxapp_install_idle_handler();
extern bool g_isIdle;
extern int g_openDialogs;

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

#ifndef __WXUNIVERSAL__
    IMPLEMENT_DYNAMIC_CLASS(wxFrame, wxTopLevelWindow)
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// GTK callbacks
// ----------------------------------------------------------------------------

#if wxUSE_MENUS_NATIVE

//-----------------------------------------------------------------------------
// "child_attached" of menu bar
//-----------------------------------------------------------------------------

static void gtk_menu_attached_callback( GtkWidget *WXUNUSED(widget), GtkWidget *WXUNUSED(child), wxFrameGTK *win )
{
    if (!win->m_hasVMT) return;

    win->m_menuBarDetached = FALSE;
    win->GtkUpdateSize();
}

//-----------------------------------------------------------------------------
// "child_detached" of menu bar
//-----------------------------------------------------------------------------

static void gtk_menu_detached_callback( GtkWidget *WXUNUSED(widget), GtkWidget *WXUNUSED(child), wxFrameGTK *win )
{
    if (!win->m_hasVMT) return;

    win->m_menuBarDetached = TRUE;
    win->GtkUpdateSize();
}

#endif // wxUSE_MENUS_NATIVE

#if wxUSE_TOOLBAR
//-----------------------------------------------------------------------------
// "child_attached" of tool bar
//-----------------------------------------------------------------------------

static void gtk_toolbar_attached_callback( GtkWidget *WXUNUSED(widget), GtkWidget *WXUNUSED(child), wxFrameGTK *win )
{
    if (!win->m_hasVMT) return;

    win->m_toolBarDetached = FALSE;

    win->GtkUpdateSize();
}

//-----------------------------------------------------------------------------
// "child_detached" of tool bar
//-----------------------------------------------------------------------------

static void gtk_toolbar_detached_callback( GtkWidget *WXUNUSED(widget), GtkWidget *WXUNUSED(child), wxFrameGTK *win )
{
    if (g_isIdle)
        wxapp_install_idle_handler();

    if (!win->m_hasVMT) return;

    win->m_toolBarDetached = TRUE;
    win->GtkUpdateSize();
}
#endif // wxUSE_TOOLBAR


// ----------------------------------------------------------------------------
// wxFrameGTK itself
// ----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// InsertChild for wxFrameGTK
//-----------------------------------------------------------------------------

/* Callback for wxFrameGTK. This very strange beast has to be used because
 * C++ has no virtual methods in a constructor. We have to emulate a
 * virtual function here as wxWindows requires different ways to insert
 * a child in container classes. */

static void wxInsertChildInFrame( wxFrameGTK* parent, wxWindow* child )
{
    wxASSERT( GTK_IS_WIDGET(child->m_widget) );

    if (!parent->m_insertInClientArea)
    {
        /* these are outside the client area */
        wxFrameGTK* frame = (wxFrameGTK*) parent;
        gtk_pizza_put( GTK_PIZZA(frame->m_mainWidget),
                         GTK_WIDGET(child->m_widget),
                         child->m_x,
                         child->m_y,
                         child->m_width,
                         child->m_height );

#if wxUSE_TOOLBAR_NATIVE
        /* we connect to these events for recalculating the client area
           space when the toolbar is floating */
        if (wxIS_KIND_OF(child,wxToolBar))
        {
            wxToolBar *toolBar = (wxToolBar*) child;
            if (toolBar->GetWindowStyle() & wxTB_DOCKABLE)
            {
                gtk_signal_connect( GTK_OBJECT(toolBar->m_widget), "child_attached",
                    GTK_SIGNAL_FUNC(gtk_toolbar_attached_callback), (gpointer)parent );

                gtk_signal_connect( GTK_OBJECT(toolBar->m_widget), "child_detached",
                    GTK_SIGNAL_FUNC(gtk_toolbar_detached_callback), (gpointer)parent );
            }
        }
#endif // wxUSE_TOOLBAR
    }
    else
    {
        /* these are inside the client area */
        gtk_pizza_put( GTK_PIZZA(parent->m_wxwindow),
                         GTK_WIDGET(child->m_widget),
                         child->m_x,
                         child->m_y,
                         child->m_width,
                         child->m_height );
    }

    /* resize on OnInternalIdle */
    parent->GtkUpdateSize();
}

// ----------------------------------------------------------------------------
// wxFrameGTK creation
// ----------------------------------------------------------------------------

void wxFrameGTK::Init()
{
    m_menuBarDetached = FALSE;
    m_toolBarDetached = FALSE;
}

bool wxFrameGTK::Create( wxWindow *parent,
                      wxWindowID id,
                      const wxString& title,
                      const wxPoint& pos,
                      const wxSize& sizeOrig,
                      long style,
                      const wxString &name )
{
    bool rt = wxTopLevelWindow::Create(parent, id, title, pos, sizeOrig, 
                                       style, name);
    m_insertCallback = (wxInsertChildFunction) wxInsertChildInFrame;
    return rt;
}

wxFrameGTK::~wxFrameGTK()
{
    m_isBeingDeleted = TRUE;
    DeleteAllBars();
}

// ----------------------------------------------------------------------------
// overridden wxWindow methods
// ----------------------------------------------------------------------------

void wxFrameGTK::DoGetClientSize( int *width, int *height ) const
{
    wxASSERT_MSG( (m_widget != NULL), wxT("invalid frame") );
    
    wxTopLevelWindow::DoGetClientSize( width, height );

    if (height)
    {
#if wxUSE_MENUS_NATIVE
        /* menu bar */
        if (m_frameMenuBar)
        {
            if (!m_menuBarDetached)
                (*height) -= wxMENU_HEIGHT;
            else
                (*height) -= wxPLACE_HOLDER;
        }
#endif // wxUSE_MENUS_NATIVE

#if wxUSE_STATUSBAR
        /* status bar */
        if (m_frameStatusBar && m_frameStatusBar->IsShown()) 
            (*height) -= wxSTATUS_HEIGHT;
#endif // wxUSE_STATUSBAR

#if wxUSE_TOOLBAR
        /* tool bar */
        if (m_frameToolBar && m_frameToolBar->IsShown())
        {
            if (m_toolBarDetached)
            {
                *height -= wxPLACE_HOLDER;
            }
            else
            {
                int x, y;
                m_frameToolBar->GetSize( &x, &y );
                if ( m_frameToolBar->GetWindowStyle() & wxTB_VERTICAL )
                {
                    *width -= x;
                }
                else
                {
                    *height -= y;
                }
            }
        }
#endif // wxUSE_TOOLBAR
    }
}

void wxFrameGTK::DoSetClientSize( int width, int height )
{
    wxASSERT_MSG( (m_widget != NULL), wxT("invalid frame") );

#if wxUSE_MENUS_NATIVE
        /* menu bar */
        if (m_frameMenuBar)
        {
            if (!m_menuBarDetached)
                height += wxMENU_HEIGHT;
            else
                height += wxPLACE_HOLDER;
        }
#endif // wxUSE_MENUS_NATIVE

#if wxUSE_STATUSBAR
        /* status bar */
        if (m_frameStatusBar && m_frameStatusBar->IsShown()) height += wxSTATUS_HEIGHT;
#endif

#if wxUSE_TOOLBAR
        /* tool bar */
        if (m_frameToolBar && m_frameToolBar->IsShown())
        {
            if (m_toolBarDetached)
            {
                height += wxPLACE_HOLDER;
            }
            else
            {
                int x, y;
                m_frameToolBar->GetSize( &x, &y );
                if ( m_frameToolBar->GetWindowStyle() & wxTB_VERTICAL )
                {
                    width += x;
                }
                else
                {
                    height += y;
                }
            }
        }
#endif

    wxTopLevelWindow::DoSetClientSize( width, height );
}

void wxFrameGTK::GtkOnSize( int WXUNUSED(x), int WXUNUSED(y),
                         int width, int height )
{
    // due to a bug in gtk, x,y are always 0
    // m_x = x;
    // m_y = y;

    /* avoid recursions */
    if (m_resizing) return;
    m_resizing = TRUE;

    /* this shouldn't happen: wxFrameGTK, wxMDIParentFrame and wxMDIChildFrame have m_wxwindow */
    wxASSERT_MSG( (m_wxwindow != NULL), wxT("invalid frame") );

    m_width = width;
    m_height = height;

    /* space occupied by m_frameToolBar and m_frameMenuBar */
    int client_area_x_offset = 0,
        client_area_y_offset = 0;

    /* wxMDIChildFrame derives from wxFrameGTK but it _is_ a wxWindow as it uses
       wxWindow::Create to create it's GTK equivalent. m_mainWidget is only
       set in wxFrameGTK::Create so it is used to check what kind of frame we
       have here. if m_mainWidget is NULL it is a wxMDIChildFrame and so we
       skip the part which handles m_frameMenuBar, m_frameToolBar and (most
       importantly) m_mainWidget */

    if ((m_minWidth != -1) && (m_width < m_minWidth)) m_width = m_minWidth;
    if ((m_minHeight != -1) && (m_height < m_minHeight)) m_height = m_minHeight;
    if ((m_maxWidth != -1) && (m_width > m_maxWidth)) m_width = m_maxWidth;
    if ((m_maxHeight != -1) && (m_height > m_maxHeight)) m_height = m_maxHeight;

    if (m_mainWidget)
    {
        /* set size hints */
        gint flag = 0; // GDK_HINT_POS;
        if ((m_minWidth != -1) || (m_minHeight != -1)) flag |= GDK_HINT_MIN_SIZE;
        if ((m_maxWidth != -1) || (m_maxHeight != -1)) flag |= GDK_HINT_MAX_SIZE;
        GdkGeometry geom;
        geom.min_width = m_minWidth;
        geom.min_height = m_minHeight;
        geom.max_width = m_maxWidth;
        geom.max_height = m_maxHeight;
        gtk_window_set_geometry_hints( GTK_WINDOW(m_widget),
                                       (GtkWidget*) NULL,
                                       &geom,
                                       (GdkWindowHints) flag );

        /* I revert back to wxGTK's original behaviour. m_mainWidget holds the
         * menubar, the toolbar and the client area, which is represented by
         * m_wxwindow.
         * this hurts in the eye, but I don't want to call SetSize()
         * because I don't want to call any non-native functions here. */

#if wxUSE_MENUS_NATIVE
        if (m_frameMenuBar)
        {
            int xx = m_miniEdge;
            int yy = m_miniEdge + m_miniTitle;
            int ww = m_width  - 2*m_miniEdge;
            int hh = wxMENU_HEIGHT;
            if (m_menuBarDetached) hh = wxPLACE_HOLDER;
            m_frameMenuBar->m_x = xx;
            m_frameMenuBar->m_y = yy;
            m_frameMenuBar->m_width = ww;
            m_frameMenuBar->m_height = hh;
            gtk_pizza_set_size( GTK_PIZZA(m_mainWidget),
                                  m_frameMenuBar->m_widget,
                                  xx, yy, ww, hh );
            client_area_y_offset += hh;
        }
#endif // wxUSE_MENUS_NATIVE

#if wxUSE_TOOLBAR
        if ((m_frameToolBar) && m_frameToolBar->IsShown() &&
            (m_frameToolBar->m_widget->parent == m_mainWidget))
        {
            int xx = m_miniEdge;
            int yy = m_miniEdge + m_miniTitle;
#if wxUSE_MENUS_NATIVE
            if (m_frameMenuBar)
            {
                if (!m_menuBarDetached)
                    yy += wxMENU_HEIGHT;
                else
                    yy += wxPLACE_HOLDER;
            }
#endif // wxUSE_MENUS_NATIVE

            m_frameToolBar->m_x = xx;
            m_frameToolBar->m_y = yy;

            /* don't change the toolbar's reported height/width */
            int ww, hh;
            if ( m_frameToolBar->GetWindowStyle() & wxTB_VERTICAL )
            {
                ww = m_toolBarDetached ? wxPLACE_HOLDER
                                       : m_frameToolBar->m_width;
                hh = m_height - 2*m_miniEdge;

                client_area_x_offset += ww;
            }
            else
            {
                ww = m_width - 2*m_miniEdge;
                hh = m_toolBarDetached ? wxPLACE_HOLDER
                                       : m_frameToolBar->m_height;

                client_area_y_offset += hh;
            }

            gtk_pizza_set_size( GTK_PIZZA(m_mainWidget),
                                  m_frameToolBar->m_widget,
                                  xx, yy, ww, hh );
        }
#endif // wxUSE_TOOLBAR

        int client_x = client_area_x_offset + m_miniEdge;
        int client_y = client_area_y_offset + m_miniEdge + m_miniTitle;
        int client_w = m_width - client_area_x_offset - 2*m_miniEdge;
        int client_h = m_height - client_area_y_offset- 2*m_miniEdge - m_miniTitle;
        gtk_pizza_set_size( GTK_PIZZA(m_mainWidget),
                              m_wxwindow,
                              client_x, client_y, client_w, client_h );
    }
    else
    {
        /* if there is no m_mainWidget between m_widget and m_wxwindow there
           is no need to set the size or position of m_wxwindow. */
    }

#if wxUSE_STATUSBAR
    if (m_frameStatusBar && m_frameStatusBar->IsShown())
    {
        int xx = 0 + m_miniEdge;
        int yy = m_height - wxSTATUS_HEIGHT - m_miniEdge - client_area_y_offset;
        int ww = m_width - 2*m_miniEdge;
        int hh = wxSTATUS_HEIGHT;
        m_frameStatusBar->m_x = xx;
        m_frameStatusBar->m_y = yy;
        m_frameStatusBar->m_width = ww;
        m_frameStatusBar->m_height = hh;
        gtk_pizza_set_size( GTK_PIZZA(m_wxwindow),
                            m_frameStatusBar->m_widget,
                            xx, yy, ww, hh );
        gtk_widget_draw( m_frameStatusBar->m_widget, (GdkRectangle*) NULL );
    }
#endif // wxUSE_STATUSBAR

    m_sizeSet = TRUE;

    // send size event to frame
    wxSizeEvent event( wxSize(m_width,m_height), GetId() );
    event.SetEventObject( this );
    GetEventHandler()->ProcessEvent( event );

#if wxUSE_STATUSBAR
    // send size event to status bar
    if (m_frameStatusBar)
    {
        wxSizeEvent event2( wxSize(m_frameStatusBar->m_width,m_frameStatusBar->m_height), m_frameStatusBar->GetId() );
        event2.SetEventObject( m_frameStatusBar );
        m_frameStatusBar->GetEventHandler()->ProcessEvent( event2 );
    }
#endif // wxUSE_STATUSBAR

    m_resizing = FALSE;
}

void wxFrameGTK::OnInternalIdle()
{
    wxTopLevelWindow::OnInternalIdle();

#if wxUSE_MENUS_NATIVE
    if (m_frameMenuBar) m_frameMenuBar->OnInternalIdle();
#endif // wxUSE_MENUS_NATIVE
#if wxUSE_TOOLBAR
    if (m_frameToolBar) m_frameToolBar->OnInternalIdle();
#endif
#if wxUSE_STATUSBAR
    if (m_frameStatusBar) m_frameStatusBar->OnInternalIdle();
#endif
}

// ----------------------------------------------------------------------------
// menu/tool/status bar stuff
// ----------------------------------------------------------------------------

#if wxUSE_MENUS_NATIVE

void wxFrameGTK::DetachMenuBar()
{
    wxASSERT_MSG( (m_widget != NULL), wxT("invalid frame") );
    wxASSERT_MSG( (m_wxwindow != NULL), wxT("invalid frame") );

    if ( m_frameMenuBar )
    {
        m_frameMenuBar->UnsetInvokingWindow( this );

        if (m_frameMenuBar->GetWindowStyle() & wxMB_DOCKABLE)
        {
            gtk_signal_disconnect_by_func( GTK_OBJECT(m_frameMenuBar->m_widget),
                GTK_SIGNAL_FUNC(gtk_menu_attached_callback), (gpointer)this );

            gtk_signal_disconnect_by_func( GTK_OBJECT(m_frameMenuBar->m_widget),
                GTK_SIGNAL_FUNC(gtk_menu_detached_callback), (gpointer)this );
        }

        gtk_container_remove( GTK_CONTAINER(m_mainWidget), m_frameMenuBar->m_widget );
        gtk_widget_ref( m_frameMenuBar->m_widget );
        gtk_widget_unparent( m_frameMenuBar->m_widget );
    }

    wxFrameBase::DetachMenuBar();
}

void wxFrameGTK::AttachMenuBar( wxMenuBar *menuBar )
{
    wxFrameBase::AttachMenuBar(menuBar);

    if (m_frameMenuBar)
    {
        m_frameMenuBar->SetInvokingWindow( this );

        m_frameMenuBar->SetParent(this);
        gtk_pizza_put( GTK_PIZZA(m_mainWidget),
                m_frameMenuBar->m_widget,
                m_frameMenuBar->m_x,
                m_frameMenuBar->m_y,
                m_frameMenuBar->m_width,
                m_frameMenuBar->m_height );

        if (menuBar->GetWindowStyle() & wxMB_DOCKABLE)
        {
            gtk_signal_connect( GTK_OBJECT(menuBar->m_widget), "child_attached",
                GTK_SIGNAL_FUNC(gtk_menu_attached_callback), (gpointer)this );

            gtk_signal_connect( GTK_OBJECT(menuBar->m_widget), "child_detached",
                GTK_SIGNAL_FUNC(gtk_menu_detached_callback), (gpointer)this );
        }

        m_frameMenuBar->Show( TRUE );
    }

    /* resize window in OnInternalIdle */
    m_sizeSet = FALSE;
}

#endif // wxUSE_MENUS_NATIVE

#if wxUSE_TOOLBAR

wxToolBar* wxFrameGTK::CreateToolBar( long style, wxWindowID id, const wxString& name )
{
    wxASSERT_MSG( (m_widget != NULL), wxT("invalid frame") );

    m_insertInClientArea = FALSE;

    m_frameToolBar = wxFrameBase::CreateToolBar( style, id, name );

    m_insertInClientArea = TRUE;

    m_sizeSet = FALSE;

    return m_frameToolBar;
}

void wxFrameGTK::SetToolBar(wxToolBar *toolbar)
{
    wxFrameBase::SetToolBar(toolbar);

    if (m_frameToolBar)
    {
        /* insert into toolbar area if not already there */
        if ((m_frameToolBar->m_widget->parent) &&
            (m_frameToolBar->m_widget->parent != m_mainWidget))
        {
            GetChildren().DeleteObject( m_frameToolBar );

            gtk_widget_reparent( m_frameToolBar->m_widget, m_mainWidget );
            GtkUpdateSize();
        }
    }
}

#endif // wxUSE_TOOLBAR

#if wxUSE_STATUSBAR

wxStatusBar* wxFrameGTK::CreateStatusBar(int number,
                                      long style,
                                      wxWindowID id,
                                      const wxString& name)
{
    wxASSERT_MSG( (m_widget != NULL), wxT("invalid frame") );

    // because it will change when toolbar is added
    m_sizeSet = FALSE;

    return wxFrameBase::CreateStatusBar( number, style, id, name );
}

void wxFrameGTK::PositionStatusBar()
{
    if ( !m_frameStatusBar )
        return;

    m_sizeSet = FALSE;
}
#endif // wxUSE_STATUSBAR

