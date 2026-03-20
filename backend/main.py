import json
import inspect
from pathlib import Path

from fastapi import FastAPI, HTTPException, WebSocket
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from fastapi.websockets import WebSocketDisconnect
from sqlmodel import Session, select

from db import create_db_and_tables, engine
from models import ArmState, CommandLog, VacuumLog, VehicleState
from mqtt_client import fast_mqtt
from ws_manager import manager
from security import JWTAuth, verify_user, TokenRequest, TokenResponse, InputValidator

BASE_DIR = Path(__file__).resolve().parent

app = FastAPI(title="Robot Arm API - Secured", version="2.0.0")
app.mount("/static", StaticFiles(directory=str(BASE_DIR / "static")), name="static")


# ============= AUTHENTICATION ENDPOINTS (Buổi 2) =============
@app.post("/auth/login", response_model=TokenResponse, tags=["Security"])
async def login(req: TokenRequest):
    """
    Login endpoint - Lấy JWT token
    Buổi 2: JWT Authentication
    
    Example:
    ```
    POST /auth/login
    {"username": "admin", "password": "admin123"}
    
    Response:
    {"access_token": "eyJ0eXAiOiJKV1QiLCJhbGc...", "token_type": "bearer", "expires_in": 86400}
    ```
    """
    # Validate input (Buổi 5)
    if not InputValidator.validate_string(req.username, max_length=50):
        raise HTTPException(status_code=400, detail="Username invalid")
    if not InputValidator.validate_string(req.password, max_length=100):
        raise HTTPException(status_code=400, detail="Password invalid")
    
    # Verify credentials
    if not verify_user(req.username, req.password):
        raise HTTPException(status_code=401, detail="Username or password incorrect")
    
    # Generate JWT token (24 hours expiration - Buổi 5)
    token = JWTAuth.create_token(user_id=req.username, username=req.username, expires_hours=24)
    
    return TokenResponse(
        access_token=token,
        token_type="bearer",
        expires_in=24 * 3600,
    )


@app.get("/dashboard", tags=["UI"])
def dashboard():
    return FileResponse(BASE_DIR / "static" / "dashboard.html")


@app.get("/control", tags=["UI"])
def control_dashboard():
    return FileResponse(BASE_DIR / "static" / "control.html")


from router import router, vehicle_router  # noqa: E402

app.include_router(router)
app.include_router(vehicle_router)


async def mqtt_publish(topic: str, payload: str):
    result = fast_mqtt.publish(topic, payload)
    if inspect.isawaitable(result):
        await result


@app.on_event("startup")
async def on_startup():
    create_db_and_tables()
    print("Database san sang")
    try:
        await fast_mqtt.mqtt_startup()
        print("MQTT startup thanh cong")
    except Exception as exc:
        print(f"MQTT startup skipped: {exc}")


@app.on_event("shutdown")
async def on_shutdown():
    try:
        await fast_mqtt.mqtt_shutdown()
    except Exception:
        pass


@fast_mqtt.on_connect()
def on_connect(client, flags, rc, properties):
    fast_mqtt.client.subscribe("servo/status")
    fast_mqtt.client.subscribe("vehicle/status")
    print(f"MQTT connected rc={rc}")


@fast_mqtt.on_disconnect()
def on_disconnect(client, packet, exc=None):
    print("MQTT disconnected")


@fast_mqtt.on_message()
async def on_message(client, topic, payload, qos, properties):
    data = payload.decode("utf-8")
    topic_str = str(topic)
    print(f"[MQTT] {topic_str}: {data}")

    try:
        obj = json.loads(data)
    except json.JSONDecodeError:
        print("MQTT payload is not valid JSON")
        return

    if topic_str == "vehicle/status":
        await _handle_vehicle(obj, data)
        return

    if topic_str == "servo/status":
        await _handle_arm(obj, data)


async def _handle_arm(obj: dict, raw: str):
    servos = obj.get("servo", [])

    def get_deg(index):
        return servos[index].get("deg") if index < len(servos) else None

    def get_pwm(index):
        return servos[index].get("pwm") if index < len(servos) else None

    vacuum_state = obj.get("vacuum", obj.get("motor", "off"))

    with Session(engine) as session:
        arm = ArmState(
            vacuum=vacuum_state,
            auto_mode=bool(obj.get("autoMode", False)),
            s0_deg=get_deg(0),
            s0_pwm=get_pwm(0),
            s1_deg=get_deg(1),
            s1_pwm=get_pwm(1),
            s2_deg=get_deg(2),
            s2_pwm=get_pwm(2),
            s3_deg=get_deg(3),
            s3_pwm=get_pwm(3),
            raw_data=raw,
        )
        session.add(arm)

        last_vacuum = session.exec(select(VacuumLog).order_by(VacuumLog.id.desc())).first()
        if not last_vacuum or last_vacuum.action != vacuum_state:
            session.add(VacuumLog(action=vacuum_state, triggered_by="mqtt"))

        session.commit()
        session.refresh(arm)

    await manager.broadcast(
        json.dumps(
            {
                "event": "arm_update",
                "db_id": arm.id,
                "vacuum": arm.vacuum,
                "auto": arm.auto_mode,
                "servos": [
                    {"ch": 0, "deg": arm.s0_deg, "pwm": arm.s0_pwm},
                    {"ch": 1, "deg": arm.s1_deg, "pwm": arm.s1_pwm},
                    {"ch": 2, "deg": arm.s2_deg, "pwm": arm.s2_pwm},
                    {"ch": 3, "deg": arm.s3_deg, "pwm": arm.s3_pwm},
                ],
            }
        )
    )


async def _handle_vehicle(obj: dict, raw: str):
    with Session(engine) as session:
        vehicle = VehicleState(
            pitch=obj.get("pitch"),
            roll=obj.get("roll"),
            cur_a=obj.get("curA"),
            cur_b=obj.get("curB"),
            cur_c=obj.get("curC"),
            cur_d=obj.get("curD"),
            cur_e=obj.get("curE"),
            is_balanced=bool(obj.get("isBalanced", False)),
            raw_data=raw,
        )
        session.add(vehicle)
        session.commit()
        session.refresh(vehicle)

    await manager.broadcast(
        json.dumps(
            {
                "event": "vehicle_update",
                "db_id": vehicle.id,
                "pitch": vehicle.pitch,
                "roll": vehicle.roll,
                "curA": vehicle.cur_a,
                "curB": vehicle.cur_b,
                "curC": vehicle.cur_c,
                "curD": vehicle.cur_d,
                "curE": vehicle.cur_e,
                "isBalanced": vehicle.is_balanced,
                "created_at": vehicle.created_at.isoformat(),
            }
        )
    )


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            data = await websocket.receive_text()
            try:
                msg = json.loads(data)
                if msg.get("type") == "command":
                    cmd = msg.get("cmd", "")
                    payload = json.dumps({"cmd": cmd})
                    await mqtt_publish("servo/command", payload)
                    with Session(engine) as session:
                        session.add(CommandLog(source="ws", command=cmd, payload=payload))
                        session.commit()
            except Exception:
                pass
    except WebSocketDisconnect:
        manager.disconnect(websocket)


@app.get("/states", tags=["Arm Database"])
def get_all_states(limit: int = 100):
    with Session(engine) as session:
        return session.exec(select(ArmState).order_by(ArmState.id.desc()).limit(limit)).all()


@app.get("/states/latest", tags=["Arm Database"])
def get_latest_state():
    with Session(engine) as session:
        return session.exec(select(ArmState).order_by(ArmState.id.desc())).first()


@app.delete("/states", tags=["Arm Database"])
def clear_all_states():
    with Session(engine) as session:
        items = session.exec(select(ArmState)).all()
        for state in items:
            session.delete(state)
        session.commit()
    return {"message": f"Da xoa {len(items)} ban ghi ArmState"}


@app.get("/vehicle/states", tags=["Vehicle Database"])
def get_vehicle_states(limit: int = 100):
    with Session(engine) as session:
        return session.exec(select(VehicleState).order_by(VehicleState.id.desc()).limit(limit)).all()


@app.get("/vehicle/latest", tags=["Vehicle Database"])
def get_vehicle_latest():
    with Session(engine) as session:
        row = session.exec(select(VehicleState).order_by(VehicleState.id.desc())).first()
        if not row:
            return {"message": "Chua co du lieu xe"}
        return row


@app.delete("/vehicle/states", tags=["Vehicle Database"])
def clear_vehicle_states():
    with Session(engine) as session:
        items = session.exec(select(VehicleState)).all()
        for vehicle in items:
            session.delete(vehicle)
        session.commit()
    return {"message": f"Da xoa {len(items)} ban ghi VehicleState"}


@app.get("/vacuum/logs", tags=["Logs"])
def get_vacuum_logs(limit: int = 50):
    with Session(engine) as session:
        return session.exec(select(VacuumLog).order_by(VacuumLog.id.desc()).limit(limit)).all()


@app.get("/commands/logs", tags=["Logs"])
def get_command_logs(limit: int = 50):
    with Session(engine) as session:
        return session.exec(select(CommandLog).order_by(CommandLog.id.desc()).limit(limit)).all()


@app.post("/command/{cmd}", tags=["Control"])
async def send_command(cmd: str):
    payload = json.dumps({"cmd": cmd})
    try:
        if not bool(getattr(fast_mqtt.client, "is_connected", False)):
            await fast_mqtt.mqtt_startup()
        await mqtt_publish("servo/command", payload)
    except Exception as exc:
        raise HTTPException(status_code=503, detail=f"MQTT chua ket noi: {exc}") from exc
    with Session(engine) as session:
        session.add(CommandLog(source="api", command=cmd, payload=payload))
        session.commit()
    return {"status": "sent", "command": cmd}


@app.post("/vehicle/command", tags=["Control"])
async def send_vehicle_command(speed: int = 0, direction: int = 0, stop: bool = False):
    payload = json.dumps({"cmd": "vehicle", "speed": speed, "direction": direction, "stop": stop})
    try:
        if not bool(getattr(fast_mqtt.client, "is_connected", False)):
            await fast_mqtt.mqtt_startup()
        await mqtt_publish("servo/command", payload)
    except Exception as exc:
        raise HTTPException(status_code=503, detail=f"MQTT chua ket noi: {exc}") from exc
    with Session(engine) as session:
        session.add(CommandLog(source="api", command="vehicle", payload=payload))
        session.commit()
    return {"status": "sent", "speed": speed, "direction": direction, "stop": stop}
