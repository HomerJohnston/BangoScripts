# Bango Scripts
### What is this?
- A level scripting engine for UE5, using blueprint graphs.
- Intended to execute small, short-lived scripted events.
- Supported on: UE5.6 - UE5.7
- Current status: ⚠️**Experimental**⚠️ - some crashes and possible data loss; do not use in a real project!
- Dependencies: None
- Pairs with: Other Bango plugins in development (Bango Facts, Bango Triggers; both pre-alpha and unreleased)
 
### Description
Have you ever placed a door, and a button, and wondered *"how do I make that button open that door?"* Have you ever placed a trigger volume after a door and wondered *"how can I conditionally spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds?"*

This plugin provides a unique system to effectively create and embed small "script" blueprints onto level actors. These blueprints simply contain a `Start` event node which you execute logic from, like many traditional game script systems. ***Bango Scripts*** also provides some extra helper-nodes to bring more traditional DSL/script features into blueprints, like explicit references to specific actors. You can either author a completely unique "level script" for a single actor, or re-usable "content scripts" in your Content folder to use in multiple places.

### Limitations
- Runtime serialization: Bango does *not* use custom graphs; this is ordinary Blueprint Graph code, using the normal Blueprint VM. Blueprint execution state is not serializable; you cannot natively save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
- Untested in multiplayer: this plugin is currently being used by the author for single-player projects; although it just uses normal blueprints, and should follow any normal rules of blueprint, so it may work fine.

### Demonstration

The following level contains:
- Two door blueprints with simple open/close events
- Several Light blueprints with simple turn on/turn off events
- One simple pressure-plate trigger (triggers when something is on top of it)
- Two simple sphere collider triggers (triggers when player walks near it)

The triggers are each set up to run a simple Bango script:

https://github.com/user-attachments/assets/94cc622d-3900-4bb5-8110-4802748d789b

### Demonstration Scripts

Here's what the Bango scripts look like, in order of appearance. First, the pressure plate. This opens the first door, and starts playing a sound track:

<img width="1145" height="860" alt="image" src="https://github.com/user-attachments/assets/d54daec8-e163-42f8-bec3-5aeb49cce0a0" />


Next, the second trigger turns on all of the lights, using some various delays. This is just a normal blueprint graph, you could set this up differently:

<img width="1978" height="1188" alt="image" src="https://github.com/user-attachments/assets/19fe4650-676c-48ed-92a4-56493d88db99" />


Finally, the third trigger, which simply opens the last door:

<img width="1147" height="863" alt="image" src="https://github.com/user-attachments/assets/b2d6fe19-f774-4f4a-ba5e-2052d374ba17" />



  
### Demonstration Video - Setting up Gameplay Demo Video
\<\<PLACEHOLDER\>\>
  
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
