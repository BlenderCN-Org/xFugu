#ifndef FGE_CONSOLEWIDGET_H
#define FGE_CONSOLEWIDGET_H

/**
 * A simple console widget.
 */

// #include <QTextEdit>

#include <QKeyEvent>
#include <Qsci/qsciscintilla.h>
#include "fglexer.h"

// Output redirector...
// from: http://www.qtforum.org/article/24554/displaying-std-cout-in-a-text-box.html

template< class Elem = char, class Tr = std::char_traits< Elem > >
  class StdRedirector : public std::basic_streambuf< Elem, Tr >
  {
      /**
        * Callback Function.
        */
    typedef void (*pfncb) ( const Elem*, std::streamsize _Count, void* pUsrData );

  public:
      /**
        * Constructor.
        * @param a_Stream the stream to redirect
        * @param a_Cb the callback function
        * @param a_pUsrData user data passed to callback
        */
    StdRedirector( std::ostream& a_Stream, pfncb a_Cb, void* a_pUsrData ) :
      m_Stream( a_Stream ),
      m_pCbFunc( a_Cb ),
      m_pUserData( a_pUsrData )
    {
        //redirect stream
      m_pBuf = m_Stream.rdbuf( this );
    };

      /**
        * Destructor.
        * Restores the original stream.
        */
    ~StdRedirector()
    {
      m_Stream.rdbuf( m_pBuf );
    }

      /**
        * Override xsputn and make it forward data to the callback function.
        */
    std::streamsize xsputn( const Elem* _Ptr, std::streamsize _Count )
    {
      m_pCbFunc( _Ptr, _Count, m_pUserData );
      return _Count;
    }

      /**
        * Override overflow and make it forward data to the callback function.
        */
    typename Tr::int_type overflow( typename Tr::int_type v )
    {
      Elem ch = Tr::to_char_type( v );
      m_pCbFunc( &ch, 1, m_pUserData );
      return Tr::not_eof( v );
    }

 protected:
    std::basic_ostream<Elem, Tr>& m_Stream;
    std::streambuf*               m_pBuf;
    pfncb                         m_pCbFunc;
    void*                         m_pUserData;
  };


class CommandLineWidget;

// Adapted from: http://code.google.com/p/qterminalwidget/source/browse/trunk/qconsolewidget.cpp

// Other refs:
// http://www.qtcentre.org/threads/17847-Design-a-commandline-widget-in-Qt4

// TODO: redirect std::cout, std::cerr to it
// see e.g., http://www.qtforum.org/article/24554/displaying-std-cout-in-a-text-box.html
// or http://lists.qt.nokia.com/pipermail/qt-interest/2011-January/030985.html

class ConsoleWidget: public QWidget {
	Q_OBJECT
public:
	ConsoleWidget(QWidget* parent = 0);
	~ConsoleWidget();

private:
    int fixedPosition;

    QsciScintilla* mConsole;

    // QsciScintilla* mCommandLine;
    CommandLineWidget* mCommandLine;
    StdRedirector<>* mStdOutRedirector;
    StdRedirector<>* mStdErrRedirector;

public slots:
	void processCommand(QString cmd);

    void textChanged();
    void cursorPositionChanged(int line, int index);
    void selectionChanged();
    void linesChanged();
signals:
	void emitCommand(QString command);

protected:
    // void keyPressEvent (QKeyEvent *e);
// public slots:
    // void cursorPositionChanged();
};

class CommandLineWidget: public QsciScintilla {
	Q_OBJECT
public:
	CommandLineWidget(QWidget* parent = 0);

public slots:
	void cursorPositionChanged(int line, int index);
	void selectionChanged();

signals:
	void emitCommand(QString command);

private:
    int fixedPosition;
protected:
	virtual void keyPressEvent(QKeyEvent* e);
};




#endif
