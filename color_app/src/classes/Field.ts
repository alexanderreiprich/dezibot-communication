import { Coordinates } from "../interfaces/Coordinates";

export class Field {

	private squares: string[][] = [];
	private fieldSize: number = 5;

	constructor(_size: number, _squares?: string[][]) {
		this.fieldSize = _size;
		// If squares are given, use those, otherwise create new ones
		if (_squares) {	
			this.setSquares(_squares);
		}
		else {
			this.createSquares(_size);
		}
	}

	// Returns copy of squares
	public getSquares(): string[][] {
		let copyOfSquares: string[][] = this.squares.map(row => {
			return row.slice();
		})
		return copyOfSquares;
	}

	public getFieldSize(): number {
		return this.fieldSize;
	}

	public setSquares(_squares: string[][]): void {
		this.squares = _squares;
	}

	// Creates squares based on given fieldsize and if it is grayscale (4 colors) or full color
	public createSquares(_size: number) {
		for (let i = 0; i < _size; i++) {
			this.squares[i] = new Array();
			for (let k = 0; k < _size; k++) {
				let color: string = "";
				let randNumber: number = Math.random();
				if (randNumber < 0.25) {
					color = "ff0000";
				}
				else if (randNumber < 0.5) {
					color = "00ff00";
				}
				else if (randNumber < 0.75) {
					color = "0000ff";
				}
				else {
					color = "ffffff";
				}
				this.squares[i][k] = color;
			}
		}
		return this.squares;
	}

	// Swap two squares
	public swapSquares(_square1: { x: number, y: number }, _square2: { x: number, y: number }): string[][] {
		const tempTile: string[][] = this.squares.map((row) => {
			return row.slice();
		});
		this.squares[_square1.x][_square1.y] = this.squares[_square2.x][_square2.y].slice();
		this.squares[_square2.x][_square2.y] = tempTile[_square1.x][_square1.y].slice();

		return this.squares;
	}

	public setSquare(_x: number, _y: number, _color: string): void {
		this.squares[_x][_y] = _color;
	}

	// Choose a random square from the current field
	public chooseRandomSquare(): Coordinates {
		let _x: number = Math.floor(Math.random() * this.fieldSize);
		let _y: number = Math.floor(Math.random() * this.fieldSize);
		return { x: _x, y: _y }
	}

	// Cycle through the colors
	public cycleColor(_color: string): string {
		switch (_color) {	
			case "ff0000":
				return "00ff00";
			case "00ff00":
				return "0000ff";
			case "0000ff":
				return "ffffff";
			default:
				return "ff0000";
		}
	}
}
