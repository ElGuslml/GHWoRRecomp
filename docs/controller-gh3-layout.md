# A controller plays as in Guitar Hero III

**Before.** Warriors of Rock gives a plain controller the vocals, so this port reports every controller as a guitar.
A controller then played as an Xbox 360 guitar does: frets on A, B, Y, X and LB, strumming on the D-pad. On a
controller that is awkward, since the thumb that strums is also needed for the frets.

**Now** (SDK change 112, `gamepad_guitar_layout = "gh3"`, the default). In a song, a controller plays as it did in
Guitar Hero III, which had a layout of its own for one, and **the game itself plays it**: Guitar Hero III's controller
code is still in Warriors of Rock (World Tour's engine kept it), unused while the game takes a controller for a guitar.
Seven hooks make the gameplay take it for a controller (`src/controller_layout.cpp`): the frets and their strum, the
note-hit rules, Star Power, the whammy bar and the frets lit on the highway. The menus and the lobby still see a
guitar, so the controller can choose one. GH III's code was read through Guitar Hero: Aerosmith's recompilation.

| | Controller |
|---|---|
| Green, red, yellow, blue, orange | LT, LB, RB, RT, A |
| Strum | None: pressing a fret plays (a chord's second fret a frame later still makes the chord: the game's own tolerance). Releasing a fret never plays. |
| Open note (bass) | B |
| Star Power | The D-pad (any direction) or Back |
| Whammy bar | Either stick, by how far it is pushed from the centre |
| Tilt | None |
| Pause | Start |

**Chords and extended sustains.** As in Guitar Hero III, a chord counts only once its frets have been let go since
the last note hit, so a chord held on into the next one does not count. Guitar Hero III asked for every fret to be let
go. Warriors of Rock also has extended sustains: a long note held while a chord is played over it. Guitar Hero III's
rule never counted those chords, because the held fret was never let go. Three more hooks ask only that the chord's
own frets were let go. A guitar's chord already leaves a sustained fret out, and so does the controller's now.

Keeping the long note was a second problem. When only some of the current note's frets are held, the game ends the
whole sustain, the long note under it too. A guitar keeps the chord held and strums it again. A controller has to let
the chord go and press it again, and a trigger and a button are never let go in exactly the same frame. That one frame
ended the long note. On the controller, while an earlier note still sustains, holding some of the current note's frets
counts as holding all of them. A fret that is let go ends its own sustain, and the long note goes on. Pressing a fret
that is not in the note still ends everything.

**In the menus and the pause menu** nothing changes: A selects, B goes back, and the D-pad moves.

**Not affected:** real guitars and drum kits, the keyboard, a controller set to Drums or Vocals, and a controller
whose whammy bar was calibrated in the launcher (the SDK takes it for a guitar).

**The launcher** (3.23): each controller playing guitar has a **Play style** selector on its Controls page, its own
(SDK change 115, `gamepad_device_guitar_layout`): **Controller · GH III** (the default) or **Guitar · Xbox 360**. As a
controller, the strum switch and the whammy bar's and tilt's rows are hidden, and the page has two groups:
- **In a song**: the five frets, the open note, Star Power and the whammy bar. Each can go on up to two of the
  controller's buttons, triggers, D-pad directions or sticks. A whammy bar on a button is either all the way or not at
  all. These are the SDK's `gamepad_song_binds` (change 113), used only while a song plays.
- **Navigation**: the Xbox 360 buttons of the menus and the pause menu, as before. An orange fret moved to Triangle
  leaves the menus' Accept where it was.

**Playing as a guitar:** **Guitar · Xbox 360** on that controller's page: it plays as an Xbox 360 guitar again, as
before this layout. That is the choice for a guitar that Windows sees as a controller (a PlayStation guitar through an
adapter), while another controller keeps Guitar Hero III's layout. `gamepad_guitar_layout = "guitar"` in `ghwor.toml`
makes every controller not named in `gamepad_device_guitar_layout` play as a guitar.

**Before the hooks** the SDK imitated the layout from outside the game. It strummed on a release and waited 20 ms to
group a chord, so chords sometimes strummed once too often. That emulation is gone.

**Status:**
- Checked in the game with a virtual Xbox 360 controller and the hooks:
  - A lit the orange fret (the guitar's code would light the green one).
  - LT and RB lit the green and yellow frets.
  - A note played by pressing its fret alone.
- Checked in the launcher: the play style selector (each controller its own), and both lists of buttons.
- Gustavo played it with a DualSense: "almost as in GH III". A chord over an extended sustain never counted, and
  pressing it again ended the long note. Both are fixed as above, and he checked them in the same song.
