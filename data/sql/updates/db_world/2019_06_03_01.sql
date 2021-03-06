-- DB update 2019_06_03_00 -> 2019_06_03_01
DROP PROCEDURE IF EXISTS `updateDb`;
DELIMITER //
CREATE PROCEDURE updateDb ()
proc:BEGIN DECLARE OK VARCHAR(100) DEFAULT 'FALSE';
SELECT COUNT(*) INTO @COLEXISTS
FROM information_schema.COLUMNS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'version_db_world' AND COLUMN_NAME = '2019_06_03_00';
IF @COLEXISTS = 0 THEN LEAVE proc; END IF;
START TRANSACTION;
ALTER TABLE version_db_world CHANGE COLUMN 2019_06_03_00 2019_06_03_01 bit;
SELECT sql_rev INTO OK FROM version_db_world WHERE sql_rev = '1559079575649059500'; IF OK <> 'FALSE' THEN LEAVE proc; END IF;
--
-- START UPDATING QUERIES
--

INSERT INTO `version_db_world` (`sql_rev`) VALUES ('1559079575649059500');

-- Generated by Event Horizon - SAI Editor (http://devsource-eventhorizon.tk/)

 -- Warlord Zol'Maz
SET @ENTRY := 28902;
SET @SOURCETYPE := 0;

DELETE FROM `smart_scripts` WHERE `entryorguid`=@ENTRY AND `source_type`=@SOURCETYPE;
UPDATE creature_template SET AIName="SmartAI" WHERE entry=@ENTRY LIMIT 1;
INSERT INTO `smart_scripts` (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES 
(@ENTRY,@SOURCETYPE,0,0,38,0,100,0,0,1,0,0,1,1,7000,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Data Set 0 1 - Say Line 1"),
(@ENTRY,@SOURCETYPE,1,2,52,0,100,0,1,28902,0,0,1,2,0,0,0,0,0,12,1,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Text 1 Over - Say Line 2"),
(@ENTRY,@SOURCETYPE,2,3,61,0,100,0,0,0,0,0,19,768,0,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Text 1 Over - Remove Unit Flags"),
(@ENTRY,@SOURCETYPE,3,4,61,0,100,0,0,0,0,0,49,0,0,0,0,0,0,12,1,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Text 1 Over - Start Attacking"),
(@ENTRY,@SOURCETYPE,4,0,61,0,100,0,0,0,0,0,9,0,0,0,0,0,0,14,57571,190784,1,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Text 1 Over - Activate Gameobject"),
(@ENTRY,@SOURCETYPE,5,0,6,0,100,0,0,0,0,0,32,0,0,0,0,0,0,14,57571,190784,1,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Just Died - Reset Gameobject"),
(@ENTRY,@SOURCETYPE,6,7,25,0,100,0,0,0,0,0,18,768,0,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Reset - Set Unit Flags"),
(@ENTRY,@SOURCETYPE,7,0,61,0,100,0,0,0,0,0,202,1,0,0,0,0,0,14,57571,190784,1,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - On Reset - Set Gameobject State 1"),
(@ENTRY,@SOURCETYPE,10,0,9,0,100,0,8,25,0,0,11,32323,0,0,0,0,0,2,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - Within Range 8-25yd - Cast Charge"),
(@ENTRY,@SOURCETYPE,11,12,2,0,100,1,0,20,0,0,11,8599,0,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - Between 0-20% Health - Cast Enrage"),
(@ENTRY,@SOURCETYPE,12,0,61,0,100,0,0,0,0,0,1,0,0,0,0,0,0,2,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - Between 0-20% Health - Say Line 0"),
(@ENTRY,@SOURCETYPE,13,0,0,0,100,0,12000,12000,20000,20000,11,54670,0,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - In Combat - Cast Decapitate"),
(@ENTRY,@SOURCETYPE,14,0,2,0,100,1,0,35,0,0,11,40546,0,0,0,0,0,1,0,0,0,0.0,0.0,0.0,0.0,"Warlord Zol'Maz - Between Health 0-35% - Cast Retaliation");

--
-- END UPDATING QUERIES
--
COMMIT;
END //
DELIMITER ;
CALL updateDb();
DROP PROCEDURE IF EXISTS `updateDb`;
