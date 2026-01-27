# Bango Scripts
### What is this?
- A level scripting engine for UE5, powered by Blueprint.
- Intended to execute small, short-lived scripted events.
- Supported on: UE5.6 - UE5.7
- Current status: ⚠️**Experimental**⚠️
- Dependencies: None
- Pairs with: Other Bango plugins in development (Bango Facts, Bango Triggers; both pre-alpha and unreleased)
 
### Description
Have you ever placed a door somewhere, and a button elsewhere, and wondered *"how do I make that button open that door?"* Have you ever placed a trigger volume inside of a door, and wondered *"how can I spawn three enemies at those markers, then turn on those lights and start playing some boss fight music after 2 seconds?"*

You could use UE's built-in `Level Blueprint`, but this is messy and hard to use, especially if your level is complex. Many teams develop messaging systems for communication between objects, but those come with other challenges:
- They can be difficult to extend with new logic (every "action" they can support has to be explicitly coded),
- They can be difficult to include latent actions or features,
- They can be slow and clunky to author complex scripted events, or bespoke one-off logic.

***Bango Scripts*** provides a unique system to effectively create and embed "script" blueprints onto level actors. These blueprints simply contain a `Start` event node which you execute logic from like a traditional game script. ***Bango Scripts*** also provides some extra helper-nodes to bring more traditional DSL/script features into blueprints, like explicit references to specific actors. You can either author a completely unique script for a single actor, or you can author a re-usable script in your Content folder and use it in multiple places.

### Limitations
- Runtime serialization: Bango does *not* use custom graphs; this is ordinary Blueprint Graph code using the normal Blueprint VM. Execution state is not serialized; you cannot natively save/restore a script back to its middle of operation (for that, you might want to use a different system like Flow Graph).
- Untested in multiplayer: this plugin is currently being used for single-player projects; although it just uses normal blueprints and should follow any normal rules of blueprint, so it may work fine if ran server-side or on clients for non-authoritative purposes.

### Demonstration Video - Gameplay
\<\<PLACEHOLDER\>\>
  
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
