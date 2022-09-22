import {Component, Input, OnInit} from '@angular/core';
import {EffectModel} from "../../models/effect.model";
import {WebsocketService} from "../../services/websocket/websocket.service";
import {ManagementService} from "../../services/management/management.service";
import {MatDialog, MatDialogRef} from "@angular/material/dialog";
import {EffectParamsComponent} from "../effect-params/effect-params.component";

@Component({
  selector: 'app-effect',
  templateUrl: './effect.component.html',
  styleUrls: ['./effect.component.scss']
})
export class EffectComponent implements OnInit {

  @Input() model!: EffectModel;
  @Input() disabled: boolean = false;

  private dialogRef: MatDialogRef<EffectParamsComponent> | null = null;

  constructor(public socketService: WebsocketService,
              public effectService: ManagementService,
              private dialog: MatDialog) {
  }

  ngOnInit(): void {
  }

  activate() {
    this.socketService.sendText(`DO:${this.model.id}`);
  }

  favorit() {
    this.model.fav = !this.model.fav;
    this.socketService.sendText(`US:${this.model.id}:${this.model.fav ? 1 : 0}`);
  }

  settings() {
    this.effectService.prepareEdit(this.model.id);
    this.dialogRef = this.dialog.open(EffectParamsComponent, {
      panelClass: 'settings-dialog-panel',
      data: {model: this.model}
    });
  }
}
