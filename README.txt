--------------------------------------
KEYBOARD AND MOUSE INPUT
--------------------------------------
Translation:
  'T' Key - change mode to translate mode and change plane on which to translate (xy/xz-plane)
  Mouse Movement - move model(s) according to movement of mouse

Rotation:
  'R' Key - change mode to rotate mode and change rotation axis to rotate around (x/y/z-axis)
  Mouse Left Click - rotate counter-clockwise by 10 degrees
  Mouse Right Click - rotate clockwise by 10 degrees

Scaling:
  'S' Key - change mode to scale mode
  Mouse Wheel Scroll Up - scale bigger by 1.2x
  Mouse Wheel Scroll Down - scale smaller by 0.8x

Loading Second Model:
  'L' Key - load second model adjacent to first model and reset all transformations on first model
  and make first model centered on the origin of world space

Color Changing:
  'C' Key - change colors of models, random colors are used for each vertex.

'Spacebar' Key - change mode to no mode
  No mouse input works.

--------------------------------------
CHANGING MODELS
--------------------------------------
To change models, modify glWindow.cpp.
Go to its OpenGLWindow::initGL() method.
Line 194 and 241 - change object names.
Choose any of the following names to change with:
  dragon.obj
  cube.obj
  teapot.obj
  suzanne.obj
  sample-bunny.obj
