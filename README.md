# Bango Scripts
### What is this?
- A level scripting engine for UE5, powered by blueprints.
- Intended for small, short-lived scripted events.
- Supported on: UE5.6 - UE5.7
- Current status: Experimental
- Dependencies: None

### Description
Have you ever placed a door somewhere, and a button elsewhere, and wondered *"how do I make that button open that door?"*

Have you ever placed a trigger volume, and wondered *"how can I spawn three enemies at those markers, then turn on those lights, and start playing some boss fight music after 2 seconds?"*

This plugin might interest you. Many developers choose to develop messaging systems to create communication between objects, but those come with challenges:
- Rigid and difficult to extend with new logic,
- Difficult to include latent features,
- Slow and clunky to author (usually filling out details panels with instanced structs or similar).

Bango Scripts provides a unique system to effectively create and embed "script blueprints" onto level actors. These blueprints simply contain a `Start` event node, and you execute your logic from that like a traditional game script. To read actors in the level in your script, simply drag the actor in from the world outliner, or copy/paste the actor right onto your script graph.

### Limitations
- Runtime serialization: Bango does *not* use custom graphs; this is (mostly) ordinary blueprint graph code. execution state of scripts is regular blueprint graph code, and is not serialized; you cannot save/restore a script back to its middle of operation.
- Untested in multiplayer: this plugin is being used for single-player development, although it just uses normal blueprint graphs and should follow any normal rules of blueprint (it should work fine if ran explicitly server-side).

## Quick-Start
- Build any sort of trigger `AActor` blueprint for your game. This could be an area-volume trigger, an event-based trigger, a button, etc.
- Add a `UBangoScriptComponent` component onto this actor.
- Using Blueprint or C++, execute the `Run` function/event on the `UBangoScriptComponent`.
- Place your trigger actor into a level and select the Script Component.
- Click the "New Level Script" button.
- Click the "Edit Script" button. A normal blueprint editor appears.
- Setup script logic off of the existing Event Start node.
- Select and copy (ctrl+c) any actor in the level editor viewport.
- Click back on your new script graph and Paste (ctrl+v) the actor onto the graph.
- Run your game! If you leave the script blueprint window open it will automatically connect to the running script when it begins running.

## Miscellaneous Features
