# Bango Scripts

Have you ever placed a door, and a button, and wondered *"how do I make that button open that door?"* 

Have you ever placed a trigger volume after a door and wondered *"how can I conditionally spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds?"*

This plugin effectively creates and embeds small "script" blueprints onto level actors. These blueprints simply contain a `Start` event node which you execute logic from, like many traditional game script systems. ***Bango Scripts*** also provides some extra helper-nodes to bring more traditional DSL/script features into blueprints, like explicit references to specific actors. You can create completely unique "level scripts" for individual actors in a level, or create re-usable "content scripts" in your Content folder to use in multiple places with input variables.

### Plugin Summary
- A level scripting engine for UE5, using blueprint graphs.
- Intended to execute small, short-lived scripted events.
- Supported on: UE5.6 - UE5.7
- Current status: ⚠️**Experimental**⚠️ *some crashes and possible data loss; do not use in a real project!*
- Dependencies: None
- Pairs with: Other Bango plugins in development (Bango Facts, Bango Triggers; both pre-alpha and unreleased)
 
### Limitations
- Runtime serialization: Bango does *not* use custom graphs; this is ordinary Blueprint Graph code, using the normal Blueprint VM. Blueprint execution state is not serializable; you cannot natively save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
- Untested in multiplayer: this plugin is currently being used by the author for single-player projects; although it just uses normal blueprints, and should follow any normal rules of blueprint, so it may work fine.

&nbsp;

# Demonstration - Gameplay

The level below contains:
- Two door blueprints with simple "open" events. The opening animation of the doors is handled by a Sequencer Actor Component on the door, and a little bit of code, in the door blueprint.
- Several Light blueprints with simple "turn on" events.
- One simple pressure-plate trigger, this contains an overlap collider box and a Bango Script component. The script component is ran on a BeginOverlap event.
- Two simple sphere collider triggers, this contains an overlap sphere and a Bango Script component. The script component is ran on a BeginOverlapy event.

The three colliders are visible at the start of the video, before entering play mode.

https://github.com/user-attachments/assets/94cc622d-3900-4bb5-8110-4802748d789b

&nbsp;

Here's what the Bango scripts look like, in order of appearance:

&nbsp;

<img width="1145" height="860" alt="image" src="https://github.com/user-attachments/assets/d54daec8-e163-42f8-bec3-5aeb49cce0a0" />

<p align="center"><i>The pressure plate script. This opens the first door, and starts playing a sound track.</i></p>

&nbsp;

<img width="1978" height="1188" alt="image" src="https://github.com/user-attachments/assets/19fe4650-676c-48ed-92a4-56493d88db99" />

<p align="center"><i>The second trigger. This turns on all of the lights, using some various delays.</i></p>

&nbsp;

<img width="1147" height="863" alt="image" src="https://github.com/user-attachments/assets/b2d6fe19-f774-4f4a-ba5e-2052d374ba17" />

<p align="center"><i>The third trigger, which simply opens the last door.</i></p>

&nbsp;

# Demonstration - Script Creation
\<\<PLACEHOLDER\>\>
  
# Quick-Start
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
