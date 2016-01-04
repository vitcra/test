<?php

/**
 * FirstLevelBehavior
 */
class FirstLevelBehavior extends LevelBehaviorBase {

  /**
   * Build the First level navigation.
   */
  public function build() {
    $fullTree = array();

    $fullTree['level1'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('menu-main level-1'))
    );
    $fullTree['level1']['menu'] = $this->getOuterLevel($this->menuTree);

    return $fullTree;
  }

}
