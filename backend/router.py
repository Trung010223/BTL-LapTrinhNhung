import json
import inspect
from datetime import datetime
from typing import List, Optional

from fastapi import APIRouter, Depends, HTTPException, Header
from pydantic import BaseModel
from sqlmodel import Session, select

from db import get_db
from models import ArmState, CommandLog, VehicleState
from mqtt_client import fast_mqtt
from security import (
    JWTAuth, 
    AESCrypto, 
    InputValidator,
    MessageSecurity,
)


# ============= AUTHENTICATION DEPENDENCY (Buổi 2) =============
def get_current_user(authorization: str = Header(None)):
    """Xác thực JWT token từ header"""
    if not authorization:
        raise HTTPException(status_code=401, detail="Missing token")
    
    try:
        scheme, token = authorization.split()
        if scheme.lower() != "bearer":
            raise HTTPException(status_code=401, detail="Invalid authorization scheme")
    except ValueError:
        raise HTTPException(status_code=401, detail="Invalid authorization header")
    
    payload = JWTAuth.verify_token(token)
    if not payload:
        raise HTTPException(status_code=401, detail="Invalid or expired token")
    
    return payload


async def publish_or_503(topic: str, payload: str):
    try:
        if not bool(getattr(fast_mqtt.client, "is_connected", False)):
            await fast_mqtt.mqtt_startup()
        result = fast_mqtt.publish(topic, payload)
        if inspect.isawaitable(result):
            await result
    except Exception as exc:
        raise HTTPException(status_code=503, detail=f"MQTT chua ket noi: {exc}") from exc


class ServoInfo(BaseModel):
    ch: int
    deg: Optional[int] = None
    pwm: Optional[int] = None


class ArmStateResponse(BaseModel):
    id: int
    vacuum: str
    auto_mode: bool
    s0_deg: Optional[int] = None
    s0_pwm: Optional[int] = None
    s1_deg: Optional[int] = None
    s1_pwm: Optional[int] = None
    s2_deg: Optional[int] = None
    s2_pwm: Optional[int] = None
    s3_deg: Optional[int] = None
    s3_pwm: Optional[int] = None
    created_at: Optional[datetime] = None

    class Config:
        from_attributes = True


class VehicleStateResponse(BaseModel):
    id: int
    pitch: Optional[float] = None
    roll: Optional[float] = None
    cur_a: Optional[int] = None
    cur_b: Optional[int] = None
    cur_c: Optional[int] = None
    cur_d: Optional[int] = None
    cur_e: Optional[int] = None
    is_balanced: bool
    created_at: Optional[datetime] = None

    class Config:
        from_attributes = True


class ServoControlRequest(BaseModel):
    ch: int
    pwm: int


class MultiServoRequest(BaseModel):
    servos: List[ServoControlRequest]


class VehicleCommandRequest(BaseModel):
    speed: int = 0
    direction: int = 0
    lift: int = 0
    stop: bool = False


router = APIRouter(
    prefix="/arm",
    tags=["Robot Arm"],
    responses={404: {"description": "Khong tim thay du lieu"}},
)

vehicle_router = APIRouter(
    prefix="/v",
    tags=["Vehicle"],
    responses={404: {"description": "Khong tim thay du lieu"}},
)


@router.get("/state", response_model=ArmStateResponse)
async def get_latest_arm_state(db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    state = db.exec(select(ArmState).order_by(ArmState.id.desc())).first()
    if not state:
        raise HTTPException(status_code=404, detail="Chua co du lieu arm")
    return state


@router.get("/history", response_model=List[ArmStateResponse])
async def get_arm_history(limit: int = 50, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    # Limit validation (Buổi 5)
    if not InputValidator.validate_number(limit, min_val=1, max_val=1000):
        raise HTTPException(status_code=400, detail="limit phai tu 1 den 1000")
    return db.exec(select(ArmState).order_by(ArmState.id.desc()).limit(limit)).all()


@router.post("/control/servo")
async def control_one_servo(req: ServoControlRequest, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    if req.ch not in range(4):
        raise HTTPException(status_code=400, detail="ch phai tu 0 den 3")
    if not (150 <= req.pwm <= 600):
        raise HTTPException(status_code=400, detail="pwm phai tu 150 den 600")

    payload = json.dumps({"cmd": "servo", "ch": req.ch, "val": req.pwm})
    await publish_or_503("servo/command", payload)

    db.add(CommandLog(source="api", command="servo", payload=payload))
    db.commit()
    return {"status": "sent", "ch": req.ch, "pwm": req.pwm}


@router.post("/control/multi")
async def control_multi_servo(req: MultiServoRequest, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    results = []
    for servo in req.servos:
        if servo.ch not in range(4):
            raise HTTPException(status_code=400, detail=f"ch={servo.ch} khong hop le")
        if not (150 <= servo.pwm <= 600):
            raise HTTPException(status_code=400, detail=f"pwm={servo.pwm} khong hop le")

        payload = json.dumps({"cmd": "servo", "ch": servo.ch, "val": servo.pwm})
        await publish_or_503("servo/command", payload)
        db.add(CommandLog(source="api", command="servo", payload=payload))
        results.append({"ch": servo.ch, "pwm": servo.pwm})

    db.commit()
    return {"status": "sent", "servos": results}


@router.post("/control/cmd")
async def send_arm_cmd(cmd: str, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    allowed = ["home", "auto", "stop", "cut"]
    if cmd not in allowed:
        raise HTTPException(status_code=400, detail=f"Lenh phai la mot trong: {allowed}")

    payload = json.dumps({"cmd": cmd})
    await publish_or_503("servo/command", payload)
    db.add(CommandLog(source="api", command=cmd, payload=payload))
    db.commit()
    return {"status": "sent", "command": cmd}


@router.delete("/history")
async def clear_arm_history(db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    items = db.exec(select(ArmState)).all()
    for state in items:
        db.delete(state)
    db.commit()
    return {"status": "success", "deleted": len(items)}


@vehicle_router.get("/state", response_model=VehicleStateResponse)
async def get_vehicle_latest(db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    row = db.exec(select(VehicleState).order_by(VehicleState.id.desc())).first()
    if not row:
        raise HTTPException(status_code=404, detail="Chua co du lieu xe")
    return row


@vehicle_router.get("/history", response_model=List[VehicleStateResponse])
async def get_vehicle_history(limit: int = 100, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    # Limit validation (Buổi 5)
    if not InputValidator.validate_number(limit, min_val=1, max_val=1000):
        raise HTTPException(status_code=400, detail="limit phai tu 1 den 1000")
    return db.exec(select(VehicleState).order_by(VehicleState.id.desc()).limit(limit)).all()


@vehicle_router.post("/command")
async def send_vehicle_cmd(req: VehicleCommandRequest, db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    if not (-255 <= req.speed <= 255):
        raise HTTPException(status_code=400, detail="speed phai tu -255 den 255")
    if req.direction not in (-1, 0, 1):
        raise HTTPException(status_code=400, detail="direction phai la -1, 0 hoac 1")
    if req.lift not in (-1, 0, 1):
        raise HTTPException(status_code=400, detail="lift phai la -1, 0 hoac 1")

    payload = json.dumps(
        {
            "cmd": "vehicle",
            "speed": req.speed,
            "direction": req.direction,
            "lift": req.lift,
            "stop": req.stop,
        }
    )
    await publish_or_503("servo/command", payload)
    db.add(CommandLog(source="api", command="vehicle", payload=payload))
    db.commit()
    return {
        "status": "sent",
        "speed": req.speed,
        "direction": req.direction,
        "lift": req.lift,
        "stop": req.stop,
    }


@vehicle_router.delete("/history")
async def clear_vehicle_history(db: Session = Depends(get_db), user: dict = Depends(get_current_user)):
    items = db.exec(select(VehicleState)).all()
    for vehicle in items:
        db.delete(vehicle)
    db.commit()
    return {"status": "success", "deleted": len(items)}
