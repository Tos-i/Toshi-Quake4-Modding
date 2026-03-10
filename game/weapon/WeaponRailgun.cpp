#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

const idEventDef EV_Railgun_RestoreHum( "<railgunRestoreHum>", "" );

class rvWeaponRailgun : public rvWeapon {
public:

	CLASS_PROTOTYPE(rvWeaponRailgun);

	rvWeaponRailgun(void);

	virtual void		Spawn(void);
	virtual void		Think(void);
	void				Save(idSaveGame* savefile) const;
	void				Restore(idRestoreGame* savefile);
	void				PreSave(void);
	void				PostSave(void);
	void				ClientUnstale(void);

protected:

	bool				UpdateAttack(void);
	bool				UpdateFlashlight(void);
	void				Flashlight(bool on);

private:

	int					chargeTime;
	int					chargeDelay;
	idVec2				chargeGlow;
	bool				fireForced;
	int					fireHeldTime;

	stateResult_t		State_Raise(const stateParms_t& parms);
	stateResult_t		State_Lower(const stateParms_t& parms);
	stateResult_t		State_Idle(const stateParms_t& parms);
	stateResult_t		State_Charge(const stateParms_t& parms);
	stateResult_t		State_Charged(const stateParms_t& parms);
	stateResult_t		State_Fire(const stateParms_t& parms);
	stateResult_t		State_Reload(const stateParms_t& parms);
	void				Event_RestoreHum(void);
	CLASS_STATES_PROTOTYPE(rvWeaponRailgun);
};

CLASS_DECLARATION(rvWeapon, rvWeaponRailgun)
END_CLASS

/*
================
rvWeaponRailgun::rvWeaponRailgun
================
*/
rvWeaponRailgun::rvWeaponRailgun(void) {
}

/*
================
rvWeaponRailgun::UpdateFlashlight
================
*/
bool rvWeaponRailgun::UpdateFlashlight(void) {
	if (!wsfl.flashlight) {
		return false;
	}

	SetState("Flashlight", 0);
	return true;
}

/*
================
rvWeaponRailgun::Flashlight
================
*/
void rvWeaponRailgun::Flashlight(bool on) {
	owner->Flashlight(on);

	if (on) {
		worldModel->ShowSurface("models/weapons/blaster/flare");
		viewModel->ShowSurface("models/weapons/blaster/flare");
	}
	else {
		worldModel->HideSurface("models/weapons/blaster/flare");
		viewModel->HideSurface("models/weapons/blaster/flare");
	}
}

/*
================
rvWeaponRailgun::UpdateAttack
================
*/
bool rvWeaponRailgun::UpdateAttack(void) {
	// Clear fire forced
	if (fireForced) {
		if (!wsfl.attack) {
			fireForced = false;
		}
		else {
			return false;
		}
	}

	// If the player is pressing the fire button and they have enough ammo for a shot
	// then start the shooting process.
	if (wsfl.attack && gameLocal.time >= nextAttackTime) {
		// Save the time which the fire button was pressed
		if (fireHeldTime == 0) {
			nextAttackTime = gameLocal.time + (fireRate * owner->PowerUpModifier(PMOD_FIRERATE));
			fireHeldTime = gameLocal.time;
			//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, chargeGlow[0]);
		}
	}

	// If they have the charge mod and they have overcome the initial charge 
	// delay then transition to the charge state.
	if (fireHeldTime != 0) {
		if (gameLocal.time - fireHeldTime > chargeDelay) {
			SetState("Charge", 4);
			return true;
		}

		// If the fire button was let go but was pressed at one point then 
		// release the shot.
		if (!wsfl.attack) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			if (player) {

				if (player->GuiActive()) {
					//make sure the player isn't looking at a gui first
					SetState("Lower", 0);
				}
				else {
					SetState("Fire", 0);
				}
			}
			return true;
		}
	}

	return false;
}

/*
================
rvWeaponRailgun::Spawn
================
*/
void rvWeaponRailgun::Spawn(void) {
	//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, 0);
	SetState("Raise", 0);

	chargeGlow = spawnArgs.GetVec2("chargeGlow");
	chargeTime = SEC2MS(spawnArgs.GetFloat("chargeTime"));
	chargeDelay = SEC2MS(spawnArgs.GetFloat("chargeDelay"));

	fireHeldTime = 0;
	fireForced = false;

	Flashlight(owner->IsFlashlightOn());
}

/*
================
rvWeaponRailgun::Save
================
*/
void rvWeaponRailgun::Save(idSaveGame* savefile) const {
	savefile->WriteInt(chargeTime);
	savefile->WriteInt(chargeDelay);
	savefile->WriteVec2(chargeGlow);
	savefile->WriteBool(fireForced);
	savefile->WriteInt(fireHeldTime);
}

/*
================
rvWeaponRailgun::Restore
================
*/
void rvWeaponRailgun::Restore(idRestoreGame* savefile) {
	savefile->ReadInt(chargeTime);
	savefile->ReadInt(chargeDelay);
	savefile->ReadVec2(chargeGlow);
	savefile->ReadBool(fireForced);
	savefile->ReadInt(fireHeldTime);
}

/*
================
rvWeaponRailgun::PreSave
================
*/
void rvWeaponRailgun::PreSave(void) {

	SetState("Idle", 4);

	StopSound(SND_CHANNEL_WEAPON, 0);
	StopSound(SND_CHANNEL_BODY, 0);
	StopSound(SND_CHANNEL_ITEM, 0);
	StopSound(SND_CHANNEL_ANY, false);

}

/*
================
rvWeaponRailgun::PostSave
================
*/
void rvWeaponRailgun::PostSave(void) {
}

/*
================
rvWeaponRailgun::Think
================
*/
void rvWeaponRailgun::Think(void) {

	// Let the real weapon think first
	rvWeapon::Think();

	if (zoomGui && wsfl.zoom && !gameLocal.isMultiplayer) {
		int ammo = AmmoInClip();
		if (ammo >= 0) {
			zoomGui->SetStateInt("player_ammo", ammo);
		}
	}
}

/*
===============================================================================

	States

===============================================================================
*/

CLASS_STATES_DECLARATION(rvWeaponRailgun)
STATE("Raise", rvWeaponRailgun::State_Raise)
STATE("Lower", rvWeaponRailgun::State_Lower)
STATE("Idle", rvWeaponRailgun::State_Idle)
STATE("Charge", rvWeaponRailgun::State_Charge)
STATE("Charged", rvWeaponRailgun::State_Charged)
STATE("Fire", rvWeaponRailgun::State_Fire)

END_CLASS_STATES

/*
================
rvWeaponRailgun::State_Raise
================
*/
stateResult_t rvWeaponRailgun::State_Raise(const stateParms_t& parms) {
	enum {
		RAISE_INIT,
		RAISE_WAIT,
	};
	switch (parms.stage) {
	case RAISE_INIT:
		SetStatus(WP_RISING);
		PlayAnim(ANIMCHANNEL_ALL, "raise", parms.blendFrames);
		return SRESULT_STAGE(RAISE_WAIT);

	case RAISE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponRailgun::State_Lower
================
*/
stateResult_t rvWeaponRailgun::State_Lower(const stateParms_t& parms) {
	enum {
		LOWER_INIT,
		LOWER_WAIT,
		LOWER_WAITRAISE
	};
	switch (parms.stage) {
	case LOWER_INIT:
		SetStatus(WP_LOWERING);
		PlayAnim(ANIMCHANNEL_ALL, "putaway", parms.blendFrames);
		return SRESULT_STAGE(LOWER_WAIT);

	case LOWER_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 0)) {
			SetStatus(WP_HOLSTERED);
			return SRESULT_STAGE(LOWER_WAITRAISE);
		}
		return SRESULT_WAIT;

	case LOWER_WAITRAISE:
		if (wsfl.raiseWeapon) {
			SetState("Raise", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponRailgun::State_Idle
================
*/
stateResult_t rvWeaponRailgun::State_Idle(const stateParms_t& parms) {
	enum {
		IDLE_INIT,
		IDLE_WAIT,
	};
	switch (parms.stage) {
	case IDLE_INIT:
		SetStatus(WP_READY);
		PlayCycle(ANIMCHANNEL_ALL, "idle", parms.blendFrames);
		return SRESULT_STAGE(IDLE_WAIT);

	case IDLE_WAIT:
		if (wsfl.lowerWeapon) {
			SetState("Lower", 4);
			return SRESULT_DONE;
		}

		if (UpdateFlashlight()) {
			return SRESULT_DONE;
		}
		if (UpdateAttack()) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponRailgun::State_Charge
================
*/
stateResult_t rvWeaponRailgun::State_Charge(const stateParms_t& parms) {
	enum {
		CHARGE_INIT,
		CHARGE_WAIT,
	};
	switch (parms.stage) {
	case CHARGE_INIT:
		//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, chargeGlow[0]);
		StartSound("snd_charge", SND_CHANNEL_ITEM, 0, false, NULL);
		PlayCycle(ANIMCHANNEL_ALL, "charging", parms.blendFrames);
		return SRESULT_STAGE(CHARGE_WAIT);

	case CHARGE_WAIT:
		if (gameLocal.time - fireHeldTime < chargeTime) {
			float f;
			f = (float)(gameLocal.time - fireHeldTime) / (float)chargeTime;
			f = chargeGlow[0] + f * (chargeGlow[1] - chargeGlow[0]);
			f = idMath::ClampFloat(chargeGlow[0], chargeGlow[1], f);
			//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, f);

			if (!wsfl.attack) {
				SetState("Fire", 0);
				return SRESULT_DONE;
			}

			return SRESULT_WAIT;
		}
		SetState("Charged", 4);
		return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponRailgun::State_Charged
================
*/
stateResult_t rvWeaponRailgun::State_Charged(const stateParms_t& parms) {
	enum {
		CHARGED_INIT,
		CHARGED_WAIT,
	};
	switch (parms.stage) {
	case CHARGED_INIT:
		//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, 1.0f);

		StopSound(SND_CHANNEL_ITEM, false);
		StartSound("snd_charge_loop", SND_CHANNEL_ITEM, 0, false, NULL);
		StartSound("snd_charge_click", SND_CHANNEL_BODY, 0, false, NULL);
		return SRESULT_STAGE(CHARGED_WAIT);

	case CHARGED_WAIT:
		if (!wsfl.attack) {
			fireForced = true;
			SetState("Fire", 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponRailgun::State_Fire
================
*/
stateResult_t rvWeaponRailgun::State_Fire(const stateParms_t& parms) {
	enum {
		FIRE_INIT,
		FIRE_WAIT,
	};
	switch (parms.stage) {
	case FIRE_INIT:

		//StopSound(SND_CHANNEL_ITEM, false);
		//viewModel->SetShaderParm(BLASTER_SPARM_CHARGEGLOW, 0);
		//don't fire if we're targeting a gui.
		idPlayer* player;
		player = gameLocal.GetLocalPlayer();

		//make sure the player isn't looking at a gui first
		if (player && player->GuiActive()) {
			fireHeldTime = 0;
			SetState("Lower", 0);
			return SRESULT_DONE;
		}

		if (player && !player->CanFire()) {
			fireHeldTime = 0;
			SetState("Idle", 4);
			return SRESULT_DONE;
		}



		if (gameLocal.time - fireHeldTime > chargeTime) {

			Attack(false, 1, spread, 0, 1.0f);
			PlayEffect("fx_chargedflash", barrelJointView, false);
			PlayAnim(ANIMCHANNEL_ALL, "chargedfire", parms.blendFrames);
			

		}
		else {
			Attack(false, 1, spread, 0, 0.2f);
			PlayEffect("fx_normalflash", barrelJointView, false);
			PlayAnim(ANIMCHANNEL_ALL, "fire", parms.blendFrames);
		}
		fireHeldTime = 0;
		

		return SRESULT_STAGE(FIRE_WAIT);

	case FIRE_WAIT:
		if (AnimDone(ANIMCHANNEL_ALL, 4)) {
			SetState("Idle", 4);
			return SRESULT_DONE;
		}
		if (UpdateFlashlight() || UpdateAttack()) {
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvWeaponRailgun::State_Reload
================
*/
stateResult_t rvWeaponRailgun::State_Reload ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( wsfl.netReload ) {
				wsfl.netReload = false;
			} else {
				NetReload ( );
			}
						
			SetStatus ( WP_RELOAD );
			PlayAnim ( ANIMCHANNEL_ALL, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				AddToClip ( ClipSize() );
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				StopSound( SND_CHANNEL_BODY2, false );
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
===============================================================================

	Event 

===============================================================================
*/

/*
================
rvWeaponRailgun::State_Reload
================
*/
void rvWeaponRailgun::Event_RestoreHum ( void ) {
	StopSound( SND_CHANNEL_BODY2, false );
	StartSound( "snd_idle_hum", SND_CHANNEL_BODY2, 0, false, NULL );
}

/*
================
rvWeaponRailgun::ClientUnStale
================
*/
void rvWeaponRailgun::ClientUnstale( void ) {
	Event_RestoreHum();
}

