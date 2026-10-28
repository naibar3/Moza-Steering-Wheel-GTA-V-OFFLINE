# Third-party notices

WheelPlugin 1.2.0 and later is under the WheelPlugin Free-Use Licence, free to use and change, not for sale (see `LICENSE`; releases up to 1.1.0 were MIT). It uses or builds on the following. None of it is relicensed by this project.

## TH Sarabun PSK (embedded font)

`src/font_data.h` contains two TrueType files (regular and bold) of **TH Sarabun PSK** as byte arrays. TH Sarabun PSK is one of the
Thai national fonts released by SIPA (Software Industry Promotion Agency, Thailand), free to use and to redistribute with its
copyright notice. The font is only rasterised in memory by the plugin (GDI+ private font collection); nothing is installed on the
user's PC. If you replace the font, keep the array names `FONT_REG`, `FONT_BOLD` and the `_LEN` constants.

## Script Hook V (not included)

Script Hook V © Alexander Blade. The SDK and the runtime must not be redistributed, so they are not in this repository or in the
install package. Download both from <http://www.dev-c.com/gtav/scripthookv/>. Plugins built with the SDK must be intended for
offline use.

## ikt32/GTAVManualTransmission (reference, not included)

Several byte patterns and the general approach for the gear / rpm / clutch / steering-input memory fields come from studying the open
source <https://github.com/ikt32/GTAVManualTransmission>. The code here is an independent implementation. If you reuse parts of this
project, check that your licence is compatible with that project's licence.

## Microsoft components

The plugin links the Windows DirectInput, GDI+, Direct3D 11 / DXGI and D3DCompiler libraries that ship with Windows. The MSVC
runtime is linked statically.
