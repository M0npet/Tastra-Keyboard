# Architecture

```text
Engine / Keyboard Model
        |
       UI
        |
Platform abstraction
        |
+----------------------+
| KWin backend         |
| input-method-v1      |
| input-panel-v1       |
+----------------------+
| wlroots backend      |
| input-method-v2      |
| virtual-keyboard-v1  |
| layer-shell          |
+----------------------+
| GNOME bridge later   |
+----------------------+
```

The core and keyboard UI must not directly depend on KWin.
