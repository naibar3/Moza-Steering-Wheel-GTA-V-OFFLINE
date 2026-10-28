WheelPlugin @VERSION@  -  steering wheel, force feedback, clutch + H-shifter, fuel and a neon dashboard for GTA V (single player)
======================================================================================================================

STORY MODE ONLY. Never use it in GTA Online. Start the game with -nobattleye.

REQUIREMENTS
  - GTA V Legacy, tested on build 1.0.3889.0 (Steam), DirectX 11
  - Script Hook V (ScriptHookV.dll + dinput8.dll) for YOUR game version: http://www.dev-c.com/gtav/scripthookv/
    (not included here: its licence does not allow redistribution)
  - A DirectInput steering wheel (force feedback optional). Made with a Moza R3.

INSTALL
  1. Install Script Hook V first (see its own readme).
  2. Close the game. Run install.bat and give it the game folder (the one with GTA5.exe),
     or copy WheelPlugin.asi and WheelPlugin.ini there yourself. NOT the OpenIV "mods" folder.
  3. Start the game WITHOUT BattlEye: -nobattleye in commandline.txt (one option per line) or in the Steam launch options.
  4. Load story mode and press F11.

FIRST START
  - About 4 seconds after the game starts the plugin pushes the wheel gently once to learn which way a force acts on it.
    KEEP YOUR HANDS OFF THE WHEEL until the message on screen says it is done.
  - F11 > "Steering and pedals": click a row, then turn the wheel / press the pedal fully to bind it.
    F11 > "General": set "Pit House rotation" to the rotation angle in your wheel software.
  - Moza Pit House: Wheel Spring Strength 0, Natural Friction / Damping low.

KEYS (all changeable in the menu, "Keyboard keys")
  F11 menu | F10 cycle Auto / Manual / Sequential | F8 engine: tap = stop, hold = start | J hold at a gas pump = refuel

THE GUIDES (complete manual, every setting, troubleshooting)
  USER_GUIDE.md      English
  USER_GUIDE.th.md   ภาษาไทย

UNINSTALL
  Close the game and run uninstall.bat, or delete WheelPlugin.asi, WheelPlugin.ini, WheelPlugin.log, WheelPlugin_pumps.txt
  and WheelPlugin_capture_*.csv from the game folder.

PROBLEMS
  Look at WheelPlugin.log in the game folder. "pattern NOT found" = the game version is not supported yet.
  Section 13 of the guide lists symptoms and fixes.

DISCLAIMER
  Not affiliated with Rockstar Games, Take-Two, Alexander Blade or Moza Racing. Use at your own risk.
  The plugin patches game memory; game updates can break it. Free to use, change and share; NOT for sale (LICENSE.txt, (c) 2026 NakimiTV). Third-party notices: THIRD_PARTY_NOTICES.txt.

-----------------------------------------------------------------------------------------------------------------------
ภาษาไทย (สรุป)
  ใช้ได้เฉพาะโหมด Story ห้ามใช้ใน GTA Online เปิดเกมด้วย -nobattleye
  ต้องมี: GTA V Legacy (ทดสอบ 1.0.3889.0) แบบ DirectX 11 และ Script Hook V ที่ตรงกับเกม (โหลดเองจากลิงก์ข้างบน ไม่ได้รวมมา)
  ติดตั้ง: ปิดเกม รัน install.bat แล้วบอกโฟลเดอร์เกม (ที่เดียวกับ GTA5.exe) หรือคัดลอก WheelPlugin.asi กับ WheelPlugin.ini ไปวางเอง
           ไม่ใช่โฟลเดอร์ mods ของ OpenIV จากนั้นเปิดเกมแบบไม่มี BattlEye เข้าโหมด Story กด F11
  เปิดครั้งแรก: ปลั๊กอินจะดันพวงมาลัยเบาๆ ครั้งเดียวเพื่อวัดทิศแรง ปล่อยมือจนกว่าข้อความบนจอจะบอกว่าเสร็จ
               F11 > พวงมาลัยและแป้น คลิกแล้วหมุน/เหยียบเพื่อ bind ตั้ง "องศาหมุนใน Pit House" ให้ตรงกับโปรแกรมพวงมาลัย
  ปุ่ม: F11 เมนู | F10 สลับโหมดเกียร์ | F8 เครื่องยนต์ (แตะ = ดับ, กดค้าง = สตาร์ท) | J กดค้างที่ปั๊ม = เติมน้ำมัน
  คู่มือฉบับสมบูรณ์: USER_GUIDE.th.md
