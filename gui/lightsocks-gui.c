/**
* Created by leer on 6/22/18.
**/

#include <stdio.h>
#include <ui.h>
#include <string.h>
#include <stdlib.h>

uiWindow *window;
uiBox *box;
uiMultilineEntry *entry;

void click_event(uiButton *btn, void *data) {
  uiMsgBox(window, "Message", uiMultilineEntryText(entry));
}

void on_add_item(uiButton *win, void *data) {
  uiBoxAppend(box, uiControl(uiNewButton(uiMultilineEntryText(entry))), 0);
}

int on_close(uiWindow *win, void *data) {
  uiQuit();
  puts("bye bye");
  return 1;
}

int main() {
  puts("hello CLion");

  uiInitOptions options;
  uiButton *btn1;
  uiButton *btn2;

  memset(&options, 0, sizeof(uiInitOptions));
  if (uiInit(&options) != NULL) {
    abort();
  }

  window = uiNewWindow("lightsocks-c", 500, 400, 0);
  uiWindowSetMargined(window, 2);

  box = uiNewVerticalBox();
  uiBoxSetPadded(box, 1);
  uiWindowSetChild(window, uiControl(box)); // add box as child

  entry = uiNewMultilineEntry();
  uiMultilineEntrySetReadOnly(entry, 0);

  btn1 = uiNewButton("Connect");
  uiButtonOnClicked(btn1, click_event, NULL);
  btn2 = uiNewButton("Add item");
  uiButtonOnClicked(btn2, on_add_item, NULL);

  uiBoxAppend(box, uiControl(entry), 0);
  uiBoxAppend(box, uiControl(btn1), 0); // add button to box
  uiBoxAppend(box, uiControl(btn2), 0); // add button to box

  uiControlShow(uiControl(window)); // show window
  uiWindowOnClosing(window, on_close, NULL);
  uiMain();
}
