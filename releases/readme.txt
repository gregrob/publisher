--------------------------------------------------
000.006.005
2021-06-06

- First release for the alarm module.
- Currently running on the alarm module.

--------------------------------------------------
000.007.005
2022-08-20

- First release to include the garage door.
- Includes test code for the ultrasonics.

--------------------------------------------------

000.007.006
2022-09-08

- The input debounce configuration was incorrectly
  set to 0.  This releases fixes this and 
  configures the reset pin to 40ms and alarm 
  sounder pin to 500ms.

--------------------------------------------------

000.007.007
2024-07-07

- Release to include pub-garage-door-active-v2
- Pin platform to espressif8266@2.6.3

- BUG: Runtime is in ms not us as variable would
       suggest

--------------------------------------------------