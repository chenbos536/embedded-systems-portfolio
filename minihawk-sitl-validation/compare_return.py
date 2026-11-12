import csv
from pathlib import Path


def peak_absolute_yaw(path):
    with path.open(newline="", encoding="utf-8") as stream:
        return max(abs(float(row["yaw_rate_deg_s"])) for row in csv.DictReader(stream))


def main():
    data = Path(__file__).resolve().parent / "data"
    direct = peak_absolute_yaw(data / "direct.csv")
    staged = peak_absolute_yaw(data / "staged.csv")
    reduction = 100 * (direct - staged) / direct
    print(f"Direct return: {direct:.2f} deg/s")
    print(f"Staged return: {staged:.2f} deg/s")
    print(f"Difference:    {reduction:.1f}% lower in the staged trial")


if __name__ == "__main__":
    main()
