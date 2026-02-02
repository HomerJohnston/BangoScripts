# Bango Scripts

Have you ever placed a door actor, and a button actor, and wondered *"how do I make that button open that door?"* 

Have you ever placed a trigger volume and wondered *"how can I conditionally spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds?"*

Imagine if you could write a short blueprint event directly onto an actor in a level. This plugin effectively allows you to do this, by creating and attaching small "script" blueprints onto actors via a custom actor component. These "script" blueprints simply contain a `Start` event node which you execute logic from, like many traditional game script systems. ***Bango Scripts*** also provides some extra helper-nodes to bring more traditional DSL/script features into blueprints, such as explicit references to specific actors in a level.

You can create completely bespoke "level scripts" for individual actors in a level, or you can create re-usable "content scripts" in your Content folder to use in multiple places, complete with input variables configurable on each instance.

### Summary Info
> [!NOTE]
> - Intended to execute small, short-lived scripted events.
> - Supported on: UE5.6 - UE5.7
> - Current status: ⚠️**Experimental**⚠️ *some crashes and possible data loss; do not use in a real project!*
 
> [!WARNING]
> - *Runtime serialization*: Bango does *not* use custom graphs; this is ordinary Blueprint Graph code, using the normal Blueprint VM. Blueprint execution state is not serializable; you cannot natively save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
> - *Untested in multiplayer*: this plugin is currently being used by the author for single-player projects; although it just uses normal blueprints, and should follow any normal rules of blueprint, so it may work fine.

> [!CAUTION]
> - *Known issues*: Be sure to visit the [Issues](https://github.com/HomerJohnston/BangoScripts/issues) page for known problems.

&nbsp;

# Demonstration - Gameplay

https://github.com/user-attachments/assets/94cc622d-3900-4bb5-8110-4802748d789b

The level above contains:
- Two door blueprints with simple "open" events. The opening animation of the doors is handled by a Sequencer Actor Component in the door blueprint, and a little bit of code.
- Several Light blueprints with simple "turn on" events.
- One simple pressure-plate trigger. This contains an overlap collider box and two Bango Script components (Activate/Deactivate). The script component is ran on a BeginOverlap event.
- Two simple sphere collider triggers. These contain an overlap sphere and a Bango Script component. The script component is ran on a BeginOverlap event, and the trigger self-destructs to prevent repeat activations.

The trigger actors are NOT related to Bango Scripts. Your game needs to supply its own reusable trigger classes.

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

# Getting Started

Go to the wiki to get started using Bango Scripts.
