/* Copyright (C) 2009, Stefan Hacker <dd0t@users.sourceforge.net>
   Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "CustomElements.h"
#include "ClientUser.h"
#include "Global.h"
#include "MainWindow.h"

/*!
  \fn int ChatbarLineEdit::completeAtCursor()
  The bar will try to complete the username, if the nickname
  is already complete it will try to find a longer match. If
  there is none it will cycle the nicknames alphabetically.
  Nothing is done on mismatch.
*/

void ChatbarLineEdit::focusInEvent(QFocusEvent *qfe) {
	inFocus(true);
	QTextEdit::focusInEvent(qfe);
}

void ChatbarLineEdit::focusOutEvent(QFocusEvent *qfe) {
	inFocus(false);
	QTextEdit::focusOutEvent(qfe);
}

void ChatbarLineEdit::inFocus(bool focus) {
	if (focus) {
		if (bDefaultVisible) {
			QFont f = font();
			f.setItalic(false);
			setFont(f);
			setPlainText(QString());
			bDefaultVisible = false;
		}
	}
	else {
		if (toPlainText().trimmed().isEmpty() || bDefaultVisible) {
			QFont f = font();
			f.setItalic(true);
			setFont(f);
			setHtml(qsDefaultText);
			bDefaultVisible = true;
		} else {
			bDefaultVisible = false;
		}
	}
}

void ChatbarLineEdit::contextMenuEvent(QContextMenuEvent *qcme) {
	QMenu *menu = createStandardContextMenu();

	QAction *action = new QAction(tr("Paste and send") + QLatin1Char('\t'), menu);
	action->setEnabled(!QApplication::clipboard()->text().isEmpty());
	connect(action, SIGNAL(triggered()), this, SLOT(pasteAndSend_triggered()));
	if (menu->actions().count() > 6)
		menu->insertAction(menu->actions()[6], action);
	else
		menu->addAction(action);

	menu->exec(qcme->globalPos());
	delete menu;
}

void ChatbarLineEdit::dropEvent(QDropEvent *event) {
	inFocus(true);
	QTextEdit::dropEvent(event);
}

ChatbarLineEdit::ChatbarLineEdit(QWidget *p) : QTextEdit(p) {
	setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setMinimumHeight(0);
	connect(this, SIGNAL(textChanged()), SLOT(doResize()));

	bDefaultVisible = true;
	setDefaultText(tr("<center>Type chat message here</center>"));
}

QSize ChatbarLineEdit::minimumSizeHint() {
	return QSize(0, fontMetrics().height());
}

QSize ChatbarLineEdit::sizeHint() {
	QSize sizeHint = QTextEdit::sizeHint();
	sizeHint.setHeight(document()->documentLayout()->documentSize().height());
	setMaximumHeight(sizeHint.height());
	return sizeHint;
}

void ChatbarLineEdit::resizeEvent(QResizeEvent *e) {
	QTextEdit::resizeEvent(e);
	doScrollbar();
}

void ChatbarLineEdit::doResize() {
	updateGeometry();
	doScrollbar();
}

void ChatbarLineEdit::doScrollbar() {
	setVerticalScrollBarPolicy(sizeHint().height() > height() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
	ensureCursorVisible();
}

void ChatbarLineEdit::setDefaultText(const QString &new_default, bool force) {
	qsDefaultText = new_default;

	if (bDefaultVisible || force) {
		QFont f = font();
		f.setItalic(true);
		setFont(f);
		setHtml(qsDefaultText);
		bDefaultVisible = true;
	}
}

bool ChatbarLineEdit::event(QEvent *event) {
	if (event->type() == QEvent::ShortcutOverride) {
		return false;
	}

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *kev = static_cast<QKeyEvent*>(event);
		if ((kev->key() == Qt::Key_Enter || kev->key() == Qt::Key_Return) && !(kev->modifiers() & Qt::ShiftModifier)) {
			g.mw->sendChatbarMessage();
			return true;
		}
// currently broken
#if 0
		if (kev->key() == Qt::Key_Tab) {
			emit tabPressed();
			return true;
		} else if (kev->key() == Qt::Key_Space && kev->modifiers() == Qt::ControlModifier) {
			emit ctrlSpacePressed();
			return true;
		}
#endif
	}
	return QTextEdit::event(event);
}

unsigned int ChatbarLineEdit::completeAtCursor() {
// currently  broken
#if 0
	// Get an alphabetically sorted list of usernames
	unsigned int id = 0;
	QList<QString> qlsUsernames;

	if (ClientUser::c_qmUsers.empty()) return id;
	foreach(ClientUser *usr, ClientUser::c_qmUsers) {
		qlsUsernames.append(usr->qsName);
	}
	qSort(qlsUsernames);

	QString newtext;
	newtext = toPlainText();
	QString target = QString();
	if (newtext.isEmpty()) {
		target = qlsUsernames.first();
		newtext = target;
	} else {
		// Get the word before the cursor
		bool bBaseIsName = false;
		int iend = cursorPosition();
		int istart = newtext.lastIndexOf(QLatin1Char(' '), iend - 1) + 1;
		QString base = newtext.mid(istart, iend - istart);

		if (qlsUsernames.last() == base) {
			bBaseIsName = true;
			target = qlsUsernames.first();
		} else {
			if (qlsUsernames.contains(base)) {
				// Prevent to complete to what's already there
				while (qlsUsernames.takeFirst() != base) {}
				bBaseIsName = true;
			}

			foreach(QString name, qlsUsernames) {
				if (name.startsWith(base, Qt::CaseInsensitive)) {
					target = name;
					break;
				}
			}
		}

		if (bBaseIsName && target.isEmpty() && !qlsUsernames.empty()) {
			// If autocomplete failed and base was a name get the next one
			target = qlsUsernames.first();
		}

		if (!target.isEmpty()) {
			newtext.replace(istart, base.length(), target);
		}
	}

	if (!target.isEmpty()) {
		foreach(ClientUser *usr, ClientUser::c_qmUsers) {
			if (usr->qsName == target) {
				id = usr->uiSession;
				break;
			}
		}
		setText(newtext);
	}
	return id;
#endif
}

void ChatbarLineEdit::pasteAndSend_triggered() {
	paste();
	g.mw->sendChatbarMessage();
}

DockTitleBar::DockTitleBar() {
	qtTick = new QTimer(this);
	qtTick->setSingleShot(true);
	connect(qtTick, SIGNAL(timeout()), this, SLOT(tick()));
	size = newsize = 0;
}

QSize DockTitleBar::sizeHint() const {
	return minimumSizeHint();
}

QSize DockTitleBar::minimumSizeHint() const {
	return QSize(size,size);
}

bool DockTitleBar::eventFilter(QObject *, QEvent *evt) {
	QDockWidget *qdw = qobject_cast<QDockWidget*>(parentWidget());

	switch (evt->type()) {
		case QEvent::Leave:
		case QEvent::Enter:
		case QEvent::MouseMove:
		case QEvent::MouseButtonRelease: {
				newsize = 0;
				QPoint p = qdw->mapFromGlobal(QCursor::pos());
				if ((p.x() >= iroundf(static_cast<float>(qdw->width()) * 0.1f)) && (p.x() < iroundf(static_cast<float>(qdw->width()) * 0.9f))  && (p.y() >= 0) && (p.y() < 15))
					newsize = 15;
				if (newsize > 0 && !qtTick->isActive())
					qtTick->start(500);
				else if ((newsize == size) && qtTick->isActive())
					qtTick->stop();
				else if (newsize == 0)
					tick();
			}
		default:
			break;
	}

	return false;
}

void DockTitleBar::tick() {
	QDockWidget *qdw = qobject_cast<QDockWidget*>(parentWidget());

	if (newsize == size)
		return;

	size = newsize;
	qdw->setTitleBarWidget(this);
}
