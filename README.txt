---Future notes---
*Cutscene animations are functional, but the animation sequences themselves need to be replaced
*Target rings get displaced slightly when they change position.  This should be fixed with an actual model, and the current ring setup will (should be) replaced with invisible colliders.  Ideally the colliders would be donut shaped (could this come in with the art model?), but if disk shaped, will need to get around displacement because  the larger rings sometimes get displaced in front of smaller ones, making it impossible to score higher points
*Tweening for camera is currently controlled by timeline, code exists in capture blueprint for using FCTween but not tested with multiple players, will need to test





--------------------
!!!!!!!!!!!!!!!!!!!
---Tasks organized by urgency---
1.Juice + polish


---Once over---
1.Arrow freezing in strange places


!!!!!!!!!!!!!!!!!!!!!!!!
------------------------


_______________________________________
How to play:

Flex or press action button to launch an arrow on your turn.  Strength of flex/time pressed on action button control distance of arrow shot.  Players go one by one, each with three arrows on their turn. However, when it is not the player's turn, they can flex/hold the action button to change the wind, which affects which direction any arrows shot will go. Wind is displayed in the upper right corner.
________________________________________




--------------
Done today:
prevent player input in cutscenes and between player turns
Familiarizing self with tween plugin for target and camera movement
Use tween for camera movements between player turns, wip
Polish vfx for when arrow hits target
Add and polish VFX for when an arrow misses + destroyed

Working on:


---------------


Bugs:

	*High priority: sometimes arrows collide and sometimes they don't. Difficult to recreate, looking for cause

	*First camera move is drastically slower than others
	
	*Applying up + down movement prevents forward movement forward prevents up + down, Last priority since it is workable 	right now. This is to simulate arrow falling on the ground
		-Maybe use tween to calculate 3/4 point, then tween down or does that also mess with physics? Possibly attempt 		after tween works with cam + targets


Polish: 	

	*Juice ideas:
		- Use FTC Tween plugin for juice effects (possibly go back and polish target and camera movement with this)


---Needed for game:---
Art assets for the targets, bow, and arrows
SFX for arrows shooting, hitting the target, and hitting the ground
Music
Shooting animation






