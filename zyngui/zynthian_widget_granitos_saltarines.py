# -*- coding: utf-8 -*-
# ******************************************************************************
# ZYNTHIAN PROJECT: Zynthian widget for "Granitos saltarines"
#
# Plugin: <https://zynthian.org/plugins/granitos_saltarines>
# Declared as a zyngui:widget LV2 UI (see the bundle's manifest.ttl).
#
# Data: LV2 output control ports ("monitors"), polled ~30 times/s while shown:
#   self.monitors[symbol] -> float
# Controls: self.processor.controllers_dict[symbol] (zctrl: .value, .set_value(v, True), .nudge(n))
#
# Copyright (C) 2026 Jofemodo
# License: GPL-3.0-or-later
# ******************************************************************************

import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base

# Output ports shown by this (generic) widget: symbol -> (label, min, max, unit)
MONITORS = {
    'grain_count': ('Grains', 0.0, 64.0, ''),
    'read_pos': ('Read position', 0.0, 1.0, ''),
    'hop_height': ('Hop height', 0.0, 1.0, ''),
}


class zynthian_widget_granitos_saltarines(zynthian_widget_base.zynthian_widget_base):

    def __init__(self, parent):
        super().__init__(parent)
        self.widget_canvas = tkinter.Canvas(self, highlightthickness=0, relief='flat',
                                            bg=zynthian_gui_config.color_bg)
        self.widget_canvas.grid(sticky='news')

        # Create all the canvas items once: refresh_gui() only moves/changes them
        self.items = {}
        tx = zynthian_gui_config.color_panel_tx
        for symbol, (label, vmin, vmax, unit) in MONITORS.items():
            self.items[symbol] = {
                'label': self.widget_canvas.create_text(0, 0, anchor=tkinter.W, text=label, fill=tx),
                'bg': self.widget_canvas.create_rectangle(0, 0, 0, 0, fill=zynthian_gui_config.color_panel_bg, width=0),
                'bar': self.widget_canvas.create_rectangle(0, 0, 0, 0, fill=zynthian_gui_config.color_on, width=0),
                'value': self.widget_canvas.create_text(0, 0, anchor=tkinter.E, text="", fill=tx),
            }
        self.bar_x0 = self.bar_x1 = 0
        self.last_values = {}

    def on_size(self, event):
        if not super().on_size(event):
            return
        n = max(1, len(MONITORS))
        row_h = min(self.height / n, self.height / 4)
        font = (zynthian_gui_config.font_family, max(8, int(row_h * 0.3)))
        self.bar_x0 = int(self.width * 0.3)
        self.bar_x1 = int(self.width * 0.8)
        for i, (symbol, items) in enumerate(self.items.items()):
            y0 = int(i * row_h + row_h * 0.25)
            y1 = int(i * row_h + row_h * 0.75)
            ym = (y0 + y1) // 2
            self.widget_canvas.coords(items['label'], 8, ym)
            self.widget_canvas.itemconfigure(items['label'], font=font)
            self.widget_canvas.coords(items['bg'], self.bar_x0, y0, self.bar_x1, y1)
            self.widget_canvas.coords(items['bar'], self.bar_x0, y0, self.bar_x0, y1)
            self.widget_canvas.coords(items['value'], self.width - 8, ym)
            self.widget_canvas.itemconfigure(items['value'], font=font)
        self.last_values = {}  # Redraw all values with the new geometry

    def refresh_gui(self):
        # Called every ~32 ms while shown: only touch the items whose value changed
        for symbol, (label, vmin, vmax, unit) in MONITORS.items():
            value = self.monitors.get(symbol)
            if value is None or value == self.last_values.get(symbol):
                continue
            self.last_values[symbol] = value
            items = self.items[symbol]
            frac = min(1.0, max(0.0, (value - vmin) / (vmax - vmin))) if vmax > vmin else 0.0
            x0, y0, x1, y1 = self.widget_canvas.coords(items['bg'])
            self.widget_canvas.coords(items['bar'], x0, y0, x0 + frac * (x1 - x0), y1)
            self.widget_canvas.itemconfigure(items['value'], text=f"{value:.1f} {unit}".strip())

    # Optional hooks (return True to consume the event, otherwise the control view handles it):
    # def zynpot_cb(self, i, dval): ...
    # def switch(self, i, t): ...
    # def cuia_arrow_up(self, params=None): ...
    # Example of a control from the widget:
    #   zctrl = self.processor.controllers_dict.get("gain"); zctrl.nudge(dval)
