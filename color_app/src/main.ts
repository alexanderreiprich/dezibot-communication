import { Field } from "./classes/Field";
import { Visual } from "./classes/Visual";

export let field: Field = new Field(5);

// Helper function for more readable code
export function getId(_id: string): HTMLElement {
	return <HTMLElement>document.getElementById(_id);
}

Visual.getInstance().update(field);
