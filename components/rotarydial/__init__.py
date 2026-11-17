"""Rotary dial (pulse dialing) support for ESPHome.

The component itself is exposed as a ``sensor`` platform, see ``sensor.py``.
"""

import esphome.codegen as cg

CODEOWNERS = ["@mvturnho"]

rotarydial_ns = cg.esphome_ns.namespace("rotarydial")
