#!/usr/bin/env python3

import csv
import re
from pathlib import Path
import numpy as np

import matplotlib.pyplot as plt


PROFILE_NAME = re.compile(r"^line(\d+)(?:t(\d+))?\.csv$")


def load_profiles(directory=Path(".")):
    profiles = {}
    expected_fields = None

    for path in directory.glob("line*.csv"):
        match = PROFILE_NAME.fullmatch(path.name)
        if match is None:
            continue

        line_number = int(match.group(1))
        time_step = int(match.group(2)) if match.group(2) is not None else None
        time_profiles = profiles.setdefault(time_step, {})
        if line_number in time_profiles:
            raise ValueError(f"Duplicate profile for line {line_number} at timestep {time_step}")

        with path.open(newline="", encoding="utf-8") as csv_file:
            reader = csv.DictReader(csv_file)
            if not reader.fieldnames or reader.fieldnames[0] != "point":
                raise ValueError(f"{path} must have a 'point' first column")
            fields = reader.fieldnames[1:]
            if not fields:
                raise ValueError(f"{path} does not contain any scalar fields")
            if expected_fields is None:
                expected_fields = fields
            elif fields != expected_fields:
                raise ValueError(
                    f"{path} has different scalar fields from the other CSV files"
                )

            rows = list(reader)
            if not rows:
                raise ValueError(f"{path} contains no sampled points")
            for row in rows:
                try:
                    int(row["point"])
                    for field in fields:
                        float(row[field])
                except (TypeError, ValueError) as error:
                    raise ValueError(f"{path} contains a non-numeric value") from error
            time_profiles[line_number] = (path, rows)

    if not profiles:
        raise FileNotFoundError("No line*.csv profiles found in the current directory")

    for time_step, time_profiles in profiles.items():
        line_numbers = sorted(time_profiles)
        expected_lines = list(range(1, line_numbers[-1] + 1))
        if line_numbers != expected_lines:
            label = "steady state" if time_step is None else f"timestep {time_step}"
            raise ValueError(f"Missing line CSV file(s) for {label}")

    return expected_fields or [], profiles


def select_fields(fields):
    print("Available Fields: " + ", ".join(fields))
    selection = input(
        "Fields to plot (space-separated, 'all' or Enter for all): "
    ).strip()
    if not selection or selection.lower() == "all":
        return fields

    selected = selection.split()
    unknown = [field for field in selected if field not in fields]
    if unknown:
        raise ValueError("Unknown field(s): " + ", ".join(unknown))
    return list(dict.fromkeys(selected))


def select_timesteps(timesteps):
    available = sorted(timesteps, key=lambda value: -1 if value is None else value)
    labels = [
        "0 (steady state)" if timestep is None else str(timestep)
        for timestep in available
    ]
    print("Available Timesteps: " + ", ".join(labels))
    selection = input(
        "Timesteps to plot (space-separated numbers or 'all'; Enter for all): "
    ).strip()
    if not selection or selection.lower() == "all":
        return available

    requested = selection.split()
    selected = []
    for token in requested:
        if not token.isdecimal():
            raise ValueError(f"Invalid timestep '{token}'; enter a timestep number or 'all'")
        number = int(token)
        timestep = None if number == 0 and None in timesteps else number
        if timestep not in timesteps:
            raise ValueError(f"Timestep {number} is not available")
        if timestep not in selected:
            selected.append(timestep)
    return selected


def main():
    fields, profiles = load_profiles()
    selected_fields = select_fields(fields)

    if not selected_fields:
        raise ValueError("The CSV files do not contain scalar fields to plot")

    selected_timesteps = select_timesteps(profiles)
    for timestep in selected_timesteps:
        for line_number in sorted(profiles[timestep]):
            path, rows = profiles[timestep][line_number]
            points = [int(row["point"]) for row in rows]
            
            x = [(points[i] - 1) * 2 / 99 for i in range(len(points))]

            plt.plot(x, [float(row['C1']) for row in rows], label=f"numerical, t =  {timestep/1000}")
            
            analytical = np.zeros(len(points))
            for i in range(len(points)):
                if x[i] <= timestep/100:
                    analytical[i] = 1
            plt.plot(x, analytical, label = f'analytical, t =  {timestep/1000}')

    plt.xlabel("x")
    plt.ylabel("Field value")

    title = "Transient simulation"
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.show()


if __name__ == "__main__":
    main()
