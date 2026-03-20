from datetime import datetime
from typing import Optional

from sqlmodel import Field, SQLModel


class ArmState(SQLModel, table=True):
    """Bảng lưu trạng thái robot arm nhận từ ESP32 qua MQTT topic: servo/status."""

    id: Optional[int] = Field(default=None, primary_key=True)
    vacuum: str = Field(default="off")
    auto_mode: bool = Field(default=False)
    s0_deg: Optional[int] = Field(default=None)
    s0_pwm: Optional[int] = Field(default=None)
    s1_deg: Optional[int] = Field(default=None)
    s1_pwm: Optional[int] = Field(default=None)
    s2_deg: Optional[int] = Field(default=None)
    s2_pwm: Optional[int] = Field(default=None)
    s3_deg: Optional[int] = Field(default=None)
    s3_pwm: Optional[int] = Field(default=None)
    raw_data: str = Field(default="")
    created_at: datetime = Field(default_factory=datetime.utcnow)


class VacuumLog(SQLModel, table=True):
    """Bảng log bật/tắt quạt hút."""

    id: Optional[int] = Field(default=None, primary_key=True)
    action: str = Field(default="off")
    triggered_by: str = Field(default="ws")
    created_at: datetime = Field(default_factory=datetime.utcnow)


class CommandLog(SQLModel, table=True):
    """Bảng log mọi lệnh điều khiển gửi xuống ESP32."""

    id: Optional[int] = Field(default=None, primary_key=True)
    source: str = Field(default="api")
    command: str = Field(default="")
    payload: str = Field(default="")
    created_at: datetime = Field(default_factory=datetime.utcnow)


class VehicleState(SQLModel, table=True):
    """Bảng lưu telemetry xe cân bằng nhận qua MQTT topic: vehicle/status."""

    id: Optional[int] = Field(default=None, primary_key=True)
    pitch: Optional[float] = Field(default=None)
    roll: Optional[float] = Field(default=None)
    cur_a: Optional[int] = Field(default=None)
    cur_b: Optional[int] = Field(default=None)
    cur_c: Optional[int] = Field(default=None)
    cur_d: Optional[int] = Field(default=None)
    cur_e: Optional[int] = Field(default=None)
    is_balanced: bool = Field(default=False)
    raw_data: str = Field(default="")
    created_at: datetime = Field(default_factory=datetime.utcnow)
